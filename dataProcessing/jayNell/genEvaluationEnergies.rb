require_relative 'base'
require_relative 'energies'
require_relative '../embeddings'

require 'date'
require 'fileutils'
require 'set'

# Write out all the energies we need to evaluate the embeddings.

EVAL_TARGETS_FILENAME = 'eval_targets.txt'
EVAL_ENERGIES_FILENAME = 'eval_energies.txt'

DATASETS_BASEDIR = File.join('..', '..', 'datasets')

L1_DISTANCE = 0
L2_DISTANCE = 1

HEAD = 0
TAIL = 1

# Uniquely combine |a| and |b|.
# Note that if |a| and |b| are ints, we must get an int back:
#   1/2 * even * odd + int  (wlog)
# = 1/2 * even + int
# = int + int
# https://en.wikipedia.org/wiki/Pairing_function#Cantor_pairing_function
def cantorPairing(a, b, int = true)
   if (int)
      return (0.5 * (a + b) * (a + b + 1)).to_i() + b
   else
      return 0.5 * (a + b) * (a + b + 1) + b
   end
end

def entityHistogram(entities, bucketSize = 10)
   puts "Total Ocurrences: #{entities.size()}"

   counts = Hash.new{|hash, key| hash[key] = 0}
   histogram = Array.new(bucketSize, 0)

   entities.each{|entity|
      counts[entity] += 1
   }

   puts "Unique Entities: #{counts.size()}"

   # Remove outliers
   # counts.delete_if{|entity, count| count > 10}

   puts "Unique Entities (No Outliers): #{counts.size()}"

   min, max = counts.to_a().map{|key, val| val}.minmax()
   puts "Min: #{min}, Max #{max}"

   counts.each_pair{|entity, count|
      val = (count - min).to_f() / (max - min)
      bucket = (val * bucketSize - 0.000001).to_i()
      histogram[bucket] += 1
   }

   step = ((max - min) / bucketSize.to_f()).ceil()
   histogram.each_index{|i|
      puts "#{"%05d" % (min + step * i)} - #{"%05d" % (min - 1 + step * (i + 1))}: #{histogram[i]}"
   }

   return
end

# Write out evaluation data for the embeddings.
# We will pick up the triples in Base::TEST_FILENAME, corrupt them, compute their energies, and write them out.
def writeEvalData(sourceDir, datasetDir, embeddingDir, embeddingMethod, distanceType)
   evaltargetsPath = File.join(embeddingDir, EVAL_TARGETS_FILENAME)
   evalEnergiesPath = File.join(embeddingDir, EVAL_ENERGIES_FILENAME)

   baseTriples = []
   File.open(File.join(datasetDir, Base::TEST_FILENAME), 'r'){|file|
      file.each{|line|
         baseTriples << line.split("\t").map{|part| part.strip().to_i()}
      }
   }

   # Get all the uinque entities and relations.
   allTriples = Energies.getTriples(sourceDir)

   entities = []
   entities += allTriples.map{|triple| triple[0]}
   entities += allTriples.map{|triple| triple[1]}
   allTriples.clear()

   entityHistogram(entities)

   entities.uniq!()
   entities.sort!

   # Keep track of what we see in the baseTriples (test set) and not just the entire corpus.
   relations = baseTriples.map{|triple| triple[2]}.uniq().sort()

   entityMapping, relationMapping = Energies.loadMappings(datasetDir)
   entityEmbeddings, relationEmbeddings = Embeddings.loadEmbeddings(embeddingDir)
   energyMethod = Embeddings.getEnergyMethod(embeddingMethod, distanceType, embeddingDir)

   # Get all the corruptions.
   # We are going to be very careful and only keep the corruptions that matter in evaluation.
   # This mean that any corrupted triple with energy greater than the target (baseTriple)
   # should not be written down.
   # However, there can be overlap in the corruptions that each target generates
   # ie. [Foo, Bar, Baz] and [Foo, Choo, Baz] will generate some of the same corruptions.
   # So, we will calculate the energy for the targets up-front and keep track of the highest energy for
   # each (head, relation) and (tail, relation) pair.

   baseEnergies = Energies.computeEnergies(
         baseTriples, entityMapping, relationMapping,
         entityEmbeddings, relationEmbeddings, energyMethod, false)

   # {HEAD => {head => {relation => cuttoffEnergy}, ...}, (same for TAIL)}.
   # Note that because of the later queries on this structure, we cannot use the auto-creation syntax.
   cuttoffEnergies = {
      HEAD => {},
      TAIL => {}
   }

   baseEnergies.each_pair{|idString, energy|
      triple = idString.split(':').map{|part| part.to_i()}

      # Insert missing keys.
      if (!cuttoffEnergies[HEAD].has_key?(triple[0]))
         cuttoffEnergies[HEAD][triple[0]] = {}
      end

      if (!cuttoffEnergies[TAIL].has_key?(triple[1]))
         cuttoffEnergies[TAIL][triple[1]] = {}
      end

      # Check to see if we have a higher skyline.
      if (!cuttoffEnergies[HEAD][triple[0]].has_key?(triple[2]) || energy > cuttoffEnergies[HEAD][triple[0]][triple[2]])
         cuttoffEnergies[HEAD][triple[0]][triple[2]] = energy
      end

      if (!cuttoffEnergies[TAIL][triple[1]].has_key?(triple[2]) || energy > cuttoffEnergies[TAIL][triple[1]][triple[2]])
         cuttoffEnergies[TAIL][triple[1]][triple[2]] = energy
      end
   }

   targetsOutFile = File.open(evaltargetsPath, 'w')
   energyOutFile = File.open(evalEnergiesPath, 'w')

   # The count of all energies we have actually written out.
   # We need this to keep a consistent surrogate key.
   energiesWritten = 0

   # All the triples we have considered.
   totalTriples = 0

   # All the energies we have computed.
   totalEnergies = 0

   # For curosity, keep track of how many triples we dropped for having too high energy.
   droppedEnergies = 0

   # We will work with one relation at a time.
   # This way we can keep a set of the triples we have already computed.
   # We do not want to recompute any triples, and at the same time we
   # want to keep our set of seen triples small so that we don't run
   # out of memory.

   # To prevent recomputation, we will leep track of the "constant entities"
   # (the entitiy that is held constant while the other one is iterated)
   # that we have seen for each relation.
   # If we are holding the TAIL constant and we see an entity that we have
   # held constant in the HEAD, then skip it.
   # Note that if the set of corruptions was complete, then we could just do some math.
   seenConstantEntitys = Set.new()
   triples = []

   relations.each{|relation|
      puts "Relation: #{relation}"

      seenConstantEntitys.clear()

      # cuttoffEnergies has a built-in list of (head, relation) and (tail, relation)
      # pairings. We just need to add the last component to generate corruptions.
      # constantEntityType will be HEAD or TAIL
      # We will need to make sure to do HEAD first.
      cuttoffEnergies.keys().sort().each{|constantEntityType|
         cuttoffEnergies[constantEntityType].each_key{|constantEntity|
            batchStartTime = (Time.now().to_f() * 1000.0).to_i()

            if (!cuttoffEnergies[constantEntityType][constantEntity].has_key?(relation))
               next
            end

            if (constantEntityType == HEAD)
               seenConstantEntitys << constantEntity
            end

            # Gather all the triples in this batch.
            entities.each{|entity|
               # Avoid duplicates.
               if (constantEntityType == TAIL && seenConstantEntitys.include?(entity))
                  next
               end

               if (constantEntityType == HEAD)
                  head = constantEntity
                  tail = entity
               else
                  head = entity
                  tail = constantEntity
               end

               triples << [head, tail, relation]
            }

            totalTriples += triples.size()

            # Process the batch
            energies = Energies.computeEnergies(
                  triples, entityMapping, relationMapping,
                  entityEmbeddings, relationEmbeddings, energyMethod,
                  false, true)

            initialSize = energies.size()
            totalEnergies += energies.size()

            energies.delete_if{|idString, energy|
               head, tail = idString.split(':').map{|part| part.to_i()}

               # Check to see if we beat the cuttoff.
               goodHead = (cuttoffEnergies[HEAD].has_key?(head) && cuttoffEnergies[HEAD][head].has_key?(relation)) && (energy <= cuttoffEnergies[HEAD][head][relation])
               goodTail = (cuttoffEnergies[TAIL].has_key?(tail) && cuttoffEnergies[TAIL][tail].has_key?(relation)) && (energy <= cuttoffEnergies[TAIL][tail][relation])

               !goodHead && !goodTail
            }

            droppedEnergies += (initialSize - energies.size())

            # Right now the energies are in a map with string key, turn into a list with a surrogate key.
            # [[index, [head, tail, relation], energy], ...]
            # No need to convert the keys to ints now, since we will just write them out.
            energies = energies.to_a().each_with_index().map{|mapEntry, index|
               head, tail = mapEntry[0].split(':')
               [index + energiesWritten, [head, tail, relation], mapEntry[1]]
            }
            energiesWritten += energies.size()

            if (energies.size() > 0)
               targetsOutFile.puts(energies.map{|energy| "#{energy[0]}\t#{energy[1].join("\t")}"}.join("\n"))
               energyOutFile.puts(energies.map{|energy| "#{energy[0]}\t#{energy[2]}"}.join("\n"))
            end

            batchEndTime = (Time.now().to_f() * 1000.0).to_i()
            puts "Batch Size: #{triples.size()} (#{energies.size()}) -- [#{batchEndTime - batchStartTime} ms]"

            energies.clear()
            triples.clear()
         }
      }
   }

   puts "Triples Considered: #{totalTriples}"
   puts "Energies Considered: #{totalEnergies}"
   puts "Energies Written: #{energiesWritten}"
   puts "Energies Dropped: #{droppedEnergies}"

   targetsOutFile.close()
   energyOutFile.close()
end

def main(args)
   sourceDir, datasetDir, embeddingDir, embeddingMethod, distanceType = parseArgs(args)
   writeEvalData(sourceDir, datasetDir, embeddingDir, embeddingMethod, distanceType)
end

def parseArgs(args)
   sourceDir = nil
   embeddingDir = nil
   datasetDir = nil
   embeddingMethod = nil
   distanceType = nil

   if (args.size() < 2 || args.size() > 5 || args.map{|arg| arg.downcase().gsub('-', '')}.include?('help'))
      puts "USAGE: ruby #{$0} <source dir> <embedding dir> [dataset dir [embedding method [distance type]]]"
      puts "Defaults:"
      puts "   dataset dir = inferred"
      puts "   embedding method = inferred"
      puts "   distance type = inferred"
      puts ""
      puts "All the inferred aguments relies on the source and emebedding directory"
      puts "being formatted by the evalAll.rb script."
      exit(2)
   end

   sourceDir = args.shift()
   embeddingDir = args.shift()

   if (args.size() > 0)
      datasetDir = args.shift()
   else
      dataset = File.basename(embeddingDir).match(/^[^_]+_(\S+)_\[size:/)[1]
      datasetDir = File.join(DATASETS_BASEDIR, File.join(dataset))
   end

   if (args.size() > 0)
      embeddingMethod = args.shift()
   else
      embeddingMethod = File.basename(embeddingDir).match(/^([^_]+)_/)[1]
   end

   if (args.size() > 0)
      distanceType = args.shift()
   else
      # TODO(eriq): This may be a little off for TransR.
      if (embeddingDir.include?("distance:#{L1_DISTANCE}"))
         distanceType = Embeddings::L1_DISTANCE_STRING
      elsif (embeddingDir.include?("distance:#{L2_DISTANCE}"))
         distanceType = Embeddings::L2_DISTANCE_STRING
      end
   end

   return sourceDir, datasetDir, embeddingDir, embeddingMethod, distanceType
end

if (__FILE__ == $0)
   main(ARGV)
end
