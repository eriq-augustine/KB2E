# Create the files that PSL will use.
# Processing:
#  - Copy over the id mapping files (not necessary, but convenient).
#  - Convert all ids in triple files from string identifiers to int identifiers.
#  - List out all the possible targets (test triples and their corruptions).
#  - Compute and output all the energy of all triples (targets).
#
# To save space (since there are typically > 400M targets), we will only write out energies
# that are less than some threshold.
# To signify an energy value that should not be included, the respective tripleEnergy() methods
# will return first return a value of false and then the actual energy value.
# We still return the actual energy so we can log it for later statistics.
#
# We make no strides to be overly efficient here, just keeping it simple.
# We will check to see if a file exists before creating it and skip that step.
# If you want a full re-run, just delete the offending directory.

require 'fileutils'
require 'set'

require './transE'
require './transH'

DEFAULT_OUT_DIR = File.join('.', 'pslData')
DATASETS_BASEDIR = File.join('..', 'datasets')

WEIGHT_EMBEDDING_BASENAME = 'weights'
ENTITY_EMBEDDING_BASENAME = 'entity2vec'
RELATION_EMBEDDING_BASENAME = 'relation2vec'

EMBEDDING_UNIF_SUFFIX = 'unif'
EMBEDDING_BERN_SUFFIX = 'bern'

ENTITY_ID_FILE = 'entity2id.txt'
RELATION_ID_FILE = 'relation2id.txt'
TEST_FILE = 'test.txt'
TRAIN_FILE = 'train.txt'
VALID_FILE = 'valid.txt'

TARGETS_FILE = 'targets.txt'
ENERGY_FILE = 'energies.txt'
ENERGY_STATS_FILE = 'energyStats.txt'

HEAD = 0
TAIL = 1
RELATION = 2

L1_DISTANCE = 0
L2_DISTANCE = 1

L1_DISTANCE_STRING = 'L1'
L2_DISTANCE_STRING = 'L2'

EMBEDDING_METHOD_TRANSE = 'TransE'
EMBEDDING_METHOD_TRANSH = 'TransH'

def copyMappings(datasetDir, outDir)
   if (!File.exists?(File.join(outDir, ENTITY_ID_FILE)))
      FileUtils.cp(File.join(datasetDir, ENTITY_ID_FILE), File.join(outDir, ENTITY_ID_FILE))
   end

   if (!File.exists?(File.join(outDir, RELATION_ID_FILE)))
      FileUtils.cp(File.join(datasetDir, RELATION_ID_FILE), File.join(outDir, RELATION_ID_FILE))
   end
end

def loadMapping(path)
   mapping = {}
   File.open(path, 'r'){|file|
      file.each{|line|
         parts = line.split("\t").map{|part| part.strip()}
         mapping[parts[0]] = parts[1]
      }
   }
   return mapping
end

def loadMappings(datasetDir)
   entityMapping = loadMapping(File.join(datasetDir, ENTITY_ID_FILE))
   relationMapping = loadMapping(File.join(datasetDir, RELATION_ID_FILE))
   return entityMapping, relationMapping
end

# Note: The if of the entity/relation is the line index in the file.
def loadEmbeddingFile(path)
   embeddings = []

   File.open(path, 'r'){|file|
      file.each{|line|
         embeddings << line.split("\t").map{|part| part.strip().to_f()}
      }
   }

   return embeddings
end

def loadWeights(embeddingDir)
   # Check if we are workig with unif or bern.
   if (File.exists?(File.join(embeddingDir, WEIGHT_EMBEDDING_BASENAME + '.' + EMBEDDING_UNIF_SUFFIX)))
      # Unif
      return loadEmbeddingFile(File.join(embeddingDir, WEIGHT_EMBEDDING_BASENAME + '.' + EMBEDDING_UNIF_SUFFIX))
   else
      # Bern
      return loadEmbeddingFile(File.join(embeddingDir, WEIGHT_EMBEDDING_BASENAME + '.' + EMBEDDING_BERN_SUFFIX))
   end
end

def loadEmbeddings(embeddingDir)
   # Check if we are workig with unif or bern.
   if (File.exists?(File.join(embeddingDir, ENTITY_EMBEDDING_BASENAME + '.' + EMBEDDING_UNIF_SUFFIX)))
      # Unif
      entityPath = File.join(embeddingDir, ENTITY_EMBEDDING_BASENAME + '.' + EMBEDDING_UNIF_SUFFIX)
      relationPath = File.join(embeddingDir, RELATION_EMBEDDING_BASENAME + '.' + EMBEDDING_UNIF_SUFFIX)
   else
      # Bern
      entityPath = File.join(embeddingDir, ENTITY_EMBEDDING_BASENAME + '.' + EMBEDDING_BERN_SUFFIX)
      relationPath = File.join(embeddingDir, RELATION_EMBEDDING_BASENAME + '.' + EMBEDDING_BERN_SUFFIX)
   end

   entityEmbeddings = loadEmbeddingFile(entityPath)
   relationEmbeddings = loadEmbeddingFile(relationPath)

   return entityEmbeddings, relationEmbeddings
end

def loadIdTriples(path)
   triples = []

   File.open(path, 'r'){|file|
      file.each{|line|
         triples << line.split("\t").map{|part| part.strip().to_i()}
      }
   }

   return triples
end

def convertIdFile(inPath, outPath, entityMapping, relationMapping)
   if (File.exists?(outPath))
      return
   end

   triples = []

   File.open(inPath, 'r'){|file|
      file.each{|line|
         parts = line.split("\t").map{|part| part.strip()}

         parts[HEAD] = entityMapping[parts[HEAD]]
         parts[RELATION] = relationMapping[parts[RELATION]]
         parts[TAIL] = entityMapping[parts[TAIL]]

         triples << parts
      }
   }

   File.open(outPath, 'w'){|file|
      file.puts(triples.map{|triple| triple.join("\t")}.join("\n"))
   }
end

def convertIds(datasetDir, outDir)
   entityMapping, relationMapping = loadMappings(datasetDir)
   convertIdFile(File.join(datasetDir, TEST_FILE), File.join(outDir, TEST_FILE), entityMapping, relationMapping)
   convertIdFile(File.join(datasetDir, TRAIN_FILE), File.join(outDir, TRAIN_FILE), entityMapping, relationMapping)
   convertIdFile(File.join(datasetDir, VALID_FILE), File.join(outDir, VALID_FILE), entityMapping, relationMapping)
end

# Make all the targets which include all the test triples and all their corruptions.
def genTargets(datasetDir, outDir)
   if (File.exists?(File.join(outDir, TARGETS_FILE)))
      return
   end

   targets = loadIdTriples(File.join(outDir, TEST_FILE))
   count = 0

   # To reduce memory consumption, we will only look at one relation at a time.
   relations = targets.map{|target| target[RELATION]}.uniq()
   numEntities = loadMapping(File.join(datasetDir, ENTITY_ID_FILE)).size()

   outFile = File.open(File.join(outDir, TARGETS_FILE), 'w')
   outFile.puts(targets.each_with_index().map{|target, i| "#{count + i}\t#{target.join("\t")}"}.join("\n"))
   count += targets.size()

   corruptions = Set.new()

   relations.each{|relation|
      targets.each{|target|
         if (target[RELATION] != relation)
            next
         end

         # Corrupt the head and tail for each triple.
         for i in 0...numEntities
            if (target[HEAD] != i)
               corruption = target.clone()
               corruption[HEAD] = i
               corruptions << corruption
            end

            if (target[TAIL] != i)
               corruption = target.clone()
               corruption[TAIL] = i
               corruptions << corruption
            end
         end
      }

      # Write out each relation's set of corruptions.
      outFile.puts(corruptions.each_with_index().map{|corruption, i| "#{count + i}\t#{corruption.join("\t")}"}.join("\n"))
      count += corruptions.size()

      corruptions.clear()
      GC.start()

      # TEST
      if (count > 100)
         break
      end
   }

   outFile.close()
end

# Compute the energy for each target.
def computeEnergies(embeddingDir, outDir, energyMethod)
   if (File.exists?(File.join(outDir, ENERGY_FILE)))
      return
   end

   entityEmbeddings, relationEmbeddings = loadEmbeddings(embeddingDir)

   # [[id, energy], ...]
   energies = []
   outFile = File.open(File.join(outDir, ENERGY_FILE), 'w')

   energyHistogram = Hash.new{|hash, key| hash[key] = 0}

   # Note that the ids in the targets are indexes into the embeddings.
   File.open(File.join(outDir, TARGETS_FILE), 'r'){|file|
      file.each{|line|
         parts = line.split("\t").map{|part| part.strip().to_i()}

         # 1 offset for id.
         ok, energy = energyMethod.call(
            entityEmbeddings[parts[1 + HEAD]],
            entityEmbeddings[parts[1 + TAIL]],
            relationEmbeddings[parts[1 + RELATION]],
            parts[1 + HEAD],
            parts[1 + TAIL],
            parts[1 + RELATION]
         )

         # Log for statistics.
         energyHistogram[energy.round(2)] += 1

         # Skip energies that are too high.
         if (!ok)
            next
         end

         energies << [
            parts[0],
            # Only output 5 places to save space.
            "%6.5f" % energy
         ]

         # Batch to minimize memory usage.
         if (energies.size() == 100000)
            outFile.puts(energies.map{|energy| energy.join("\t")}.join("\n"))
            energies.clear()
         end
      }
   }

   if (energies.size() != 0)
      outFile.puts(energies.map{|energy| energy.join("\t")}.join("\n"))
      energies.clear()
   end

   outFile.close()

   writeEnergyStats(energyHistogram, outDir)
end

def writeEnergyStats(energyHistogram, outDir)
   tripleCount = energyHistogram.values().reduce(0, :+)
   mean = energyHistogram.each_pair().map{|energy, count| energy * count}.reduce(0, :+) / tripleCount.to_f()
   variance = energyHistogram.each_pair().map{|energy, count| count * ((energy - mean) ** 2)}.reduce(0, :+) / tripleCount.to_f()
   stdDev = Math.sqrt(variance)
   min = energyHistogram.keys().min()
   max = energyHistogram.keys().max()
   range = max - min

   # Keep track of the counts in each quartile.
   quartileCounts = [0, 0, 0, 0]
   energyHistogram.each_pair().each{|energy, count|
      # The small subtraction is to offset the max.
      quartile = (((energy - min - 0.0000001).to_f() / range) * 100).to_i() / 25
      quartileCounts[quartile] += count
   }

   # Calculate the median.
   # HACK(eriq): This is slighty off if there is an even number of triples and the
   # two median values are on a break, but it is not worth the extra effort.
   median = -1
   totalCount = 0
   energyHistogram.each_pair().sort().each{|energy, count|
      totalCount += count

      if (totalCount >= (tripleCount / 2))
         median = energy
         break
      end
   }

   File.open(File.join(outDir, ENERGY_STATS_FILE), 'w'){|file|
      file.puts "Num Triples: #{energyHistogram.size()}"
      file.puts "Num Unique Energies: #{tripleCount}"
      file.puts "Min Energy: #{energyHistogram.keys().min()}"
      file.puts "Max Energy: #{energyHistogram.keys().max()}"
      file.puts "Quartile Counts: #{quartileCounts}"
      file.puts "Quartile Percentages: #{quartileCounts.map{|count| (count / tripleCount.to_f()).round(2)}}"
      file.puts "Mean Energy: #{mean}"
      file.puts "Median Energy: #{median}"
      file.puts "Energy Variance: #{variance}"
      file.puts "Energy StdDev: #{stdDev}"
      file.puts "---"
      file.puts energyHistogram.each_pair().sort().map{|pair| pair.join("\t")}.join("\n")
   }
end

def parseArgs(args)
   embeddingDir = nil
   outDir = nil
   datasetDir = nil
   embeddingMethod = nil
   distanceType = nil

   if (args.size() < 1 || args.size() > 5 || args.map{|arg| arg.downcase().gsub('-', '')}.include?('help'))
      puts "USAGE: ruby #{$0} embedding dir [output dir [dataset dir [embedding method [distance type]]]]"
      puts "Defaults:"
      puts "   output dir = inferred"
      puts "   dataset dir = inferred"
      puts "   embedding method = inferred"
      puts "   distance type = inferred"
      puts ""
      puts "All the inferred aguments relies on the emebedding directory"
      puts "being formatted by the evalAll.rb script."
      puts "The directory that the inferred output directory will be put in is: #{DEFAULT_OUT_DIR}."
      exit(2)
   end

   if (args.size() > 0)
      embeddingDir = args[0]
   end

   if (args.size() > 1)
      outDir = args[1]
   else
      outDir = File.join(DEFAULT_OUT_DIR, File.basename(embeddingDir))
   end

   if (args.size() > 2)
      datasetDir = args[2]
   else
      dataset = File.basename(embeddingDir).match(/^[^_]+_([^\[]+)_\[/)[1]
      datasetDir = File.join(DATASETS_BASEDIR, File.join(dataset))
   end

   if (args.size() > 3)
      embeddingMethod = args[3]
   else
      embeddingMethod = File.basename(embeddingDir).match(/^([^_]+)_/)[1]
   end

   if (args.size() > 4)
      distanceType = args[4]
   else
      # TODO(eriq): This may be a little off for TransR.
      if (embeddingDir.include?("distance:#{L1_DISTANCE}"))
         distanceType = L1_DISTANCE_STRING
      elsif (embeddingDir.include?("distance:#{L2_DISTANCE}"))
         distanceType = L2_DISTANCE_STRING
      end
   end

   energyMethod = getEnergyMethod(embeddingMethod, distanceType, embeddingDir)

   return datasetDir, embeddingDir, outDir, energyMethod
end

# Given an embedding method and distance type, return a proc that will compute the energy.
def getEnergyMethod(embeddingMethod, distanceType, embeddingDir)
   if (![L1_DISTANCE_STRING, L2_DISTANCE_STRING].include?(distanceType))
      raise("Unknown distance type: #{distanceType}")
   end

   case embeddingMethod
   when EMBEDDING_METHOD_TRANSE
      return proc{|head, tail, relation, headId, tailId, relationId|
         TransE.tripleEnergy(distanceType, head, tail, relation)
      }
   when EMBEDDING_METHOD_TRANSH
      transHWeights = loadWeights(embeddingDir)
      return proc{|head, tail, relation, headId, tailId, relationId|
         TransH.tripleEnergy(head, tail, relation, transHWeights[relationId])
      }
   else
      $stderr.puts("Unknown embedding method: #{embeddingMethod}")
      exit(4)
   end
end

def prepForPSL(args)
   datasetDir, embeddingDir, outDir, energyMethod = parseArgs(args)

   FileUtils.mkdir_p(outDir)

   copyMappings(datasetDir, outDir)
   convertIds(datasetDir, outDir)
   genTargetsEnerg(datasetDir, outDir)
   computeEnergies(embeddingDir, outDir, energyMethod)
end

if (__FILE__ == $0)
   prepForPSL(ARGV)
end
