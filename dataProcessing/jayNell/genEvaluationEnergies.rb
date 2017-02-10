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

   # TEST
   # if (File.exists?(evaltargetsPath) && File.exists?(evalEnergiesPath))
   #    return
   # end

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

   # Keep track of what we see in the baseTriples (test set) and not just the entire corpus.
   relations = baseTriples.map{|triple| triple[2]}.uniq()

   entityMapping, relationMapping = Energies.loadMappings(datasetDir)
   entityEmbeddings, relationEmbeddings = Embeddings.loadEmbeddings(embeddingDir)
   energyMethod = Embeddings.getEnergyMethod(embeddingMethod, distanceType, embeddingDir)

   # Get all the corruptions.
   # We are going to be very careful and only keep the corruptions that matter in evaluation.
   # This mean that any corrupted triple with energy greater than the target (baseTriple)
   # should not be written down.
   # However, there can be overlap in the corruptions that each target generates
   # ie. [Foo, Bar, Baz] and [Foo, Choo, Baz] will generate the same corruptions.
   # So, we will calculate the energy for the targets up-front and keep track of the highest energy for
   # each (head, relation) and (tail, relation) pair.

   baseEnergies = Energies.computeEnergies(
         baseTriples, entityMapping, relationMapping,
         entityEmbeddings, relationEmbeddings, energyMethod, false)

   # {HEAD => {head => {relation => cuttoffEnergy}, ...}, (same for TAIL)}.
   cuttoffEnergies = {
      HEAD => Hash.new{|entityHash, entityKey|
         entityHash[entityKey] = Hash.new{|relationHash, relationKey|
            relationHash[relationKey] = -1
         }
      },
      TAIL => Hash.new{|entityHash, entityKey|
         entityHash[entityKey] = Hash.new{|relationHash, relationKey|
            relationHash[relationKey] = -1
         }
      }
   }

   baseEnergies.each_pair{|idString, energy|
      triple = idString.split(':').map{|part| part.to_i()}

      if (energy > cuttoffEnergies[HEAD][triple[0]][triple[2]])
         cuttoffEnergies[HEAD][triple[0]][triple[2]] = energy
      end

      if (energy > cuttoffEnergies[TAIL][triple[1]][triple[2]])
         cuttoffEnergies[TAIL][triple[1]][triple[2]] = energy
      end
   }

   targetsOutFile = File.open(evaltargetsPath, 'w')
   energyOutFile = File.open(evalEnergiesPath, 'w')
   # The count of all energies we have actually written out.
   # We need this to keep a consistent surrogate key.
   energiesWritten = 0

   # All the energies we have computed.
   totalEnergies = 0

   # For curosity, keep track of how many triples we dropped for having too high energy.
   droppedEnergies = 0

   # We will work with one relation at a time.
   # This way we can keep a set of the triples we have already computed.
   # We do not want to recompute any triples, and at the same time we
   # want to keep our set of seen triples small so that we don't run
   # out of memory.
   seenTriples = Set.new()
   triples = []

   relations.each{|relation|
      seenTriples.clear()

      # cuttoffEnergies has a built-in list of (head, relation) and (tail, relation)
      # pairings. We just need to add the last component to generate corruptions.
      # constantEntityType will be HEAD or TAIL
      cuttoffEnergies.each_key{|constantEntityType|
         cuttoffEnergies[constantEntityType].each_key{|constantEntity|
            if (!cuttoffEnergies[constantEntityType][constantEntity].has_key?(relation))
               next
            end

            entities.each{|entity|
               if (constantEntityType == HEAD)
                  head = constantEntity
                  tail = entity
               else
                  head = entity
                  tail = constantEntity
               end

               id = "#{head}:#{tail}"
               if (!seenTriples.include?(id))
                  triples << [head, tail, relation]

                  # We only need to keep track of the head and tail since we are clearing
                  # every relation.
                  seenTriples << id
               end
            }

            puts "Batch Size: #{triples.size()}"

            totalEnergies += triples.size()

            # Process the batch
            energies = Energies.computeEnergies(
                  triples, entityMapping, relationMapping,
                  entityEmbeddings, relationEmbeddings, energyMethod, false)

            triples.clear()
            initialSize = energies.size()

            energies.delete_if{|idString, energy|
               triple = idString.split(':').map{|part| part.to_i()}

               # Check to see if we beat the cuttoff.
               goodHead = energy <= cuttoffEnergies[HEAD][triple[0]][triple[2]]
               goodTail = energy <= cuttoffEnergies[TAIL][triple[1]][triple[2]]

               !goodHead && !goodTail
            }

            droppedEnergies += (initialSize - energies.size())

            # Right now the energies are in a map with string key, turn into a list with a surrogate key.
            # [[index, [head, tail, relation], energy], ...]
            # No need to convert the keys to ints now, since we will just write them out.
            energies = energies.to_a().each_with_index().map{|mapEntry, index|
               [index + energiesWritten, mapEntry[0].split(':'), mapEntry[1]]
            }
            energiesWritten += energies.size()

            if (energies.size() > 0)
               targetsOutFile.puts(energies.map{|energy| "#{energy[0]}\t#{energy[1].join("\t")}"}.join("\n"))
               energyOutFile.puts(energies.map{|energy| "#{energy[0]}\t#{energy[2]}"}.join("\n"))
            end
            
            energies.clear()
            GC.start()
         }
      }
   }

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
