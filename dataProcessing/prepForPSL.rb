# Create the files that PSL will use.
# Processing:
#  - Copy over the id mapping files (not necessary, but convenient).
#  - Convert all ids in triple files from string identifiers to int identifiers.
#  - List out all the possible targets (test triples and their corruptions).
#  - Compute and output all the energy of all triples (targets).
#
# We make no strides to be overly efficient here, just keeping it simple.

require 'fileutils'
require 'set'

require './distance'
require './transE'

DEFAULT_DATASET_DIR = File.join('..', 'datasets', 'FB15k')
DEFAULT_EMBEDDING_DIR = File.join('..', 'evaluationData', 'TransE_FB15k_[size:100,margin:1,method:1,rate:0.01,batches:100,epochs:1000,distance:1]')
DEFAULT_OUT_DIR = File.join('.', File.basename(DEFAULT_EMBEDDING_DIR))

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
   FileUtils.cp(File.join(datasetDir, ENTITY_ID_FILE), File.join(outDir, ENTITY_ID_FILE))
   FileUtils.cp(File.join(datasetDir, RELATION_ID_FILE), File.join(outDir, RELATION_ID_FILE))
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
   }

   outFile.close()
end

# Compute the energy for each target.
def computeEnergies(embeddingDir, outDir, energyMethod)
   entityEmbeddings, relationEmbeddings = loadEmbeddings(embeddingDir)

   # [[id, energy], ...]
   energies = []
   outFile = File.open(File.join(outDir, ENERGY_FILE), 'w')

   # Note that the ids in the targets are indexes into the embeddings.
   File.open(File.join(outDir, TARGETS_FILE), 'r'){|file|
      file.each{|line|
         parts = line.split("\t").map{|part| part.strip().to_i()}

         # 1 offset for id.
         energies << [
            parts[0],
            # Only output 5 places to save space.
            "%6.5f" % energyMethod.call(
               entityEmbeddings[parts[1 + HEAD]],
               entityEmbeddings[parts[1 + TAIL]],
               relationEmbeddings[parts[1 + RELATION]]
            )
         ]
      }

      # Batch to minimize memory usage.
      if (energies.size() != 0)
         outFile.puts(energies.map{|energy| energy.join("\t")}.join("\n"))
         energies.clear()
      end
   }

   if (energies.size() != 0)
      outFile.puts(energies.map{|energy| energy.join("\t")}.join("\n"))
      energies.clear()
   end

   outFile.close()
end

def parseArgs(args)
   datasetDir = DEFAULT_DATASET_DIR
   embeddingDir = DEFAULT_EMBEDDING_DIR
   outDir = DEFAULT_OUT_DIR

   embeddingMethod = nil
   distanceType = nil

   if (args.size() > 3 || args.map{|arg| arg.downcase().gsub('-', '')}.include?('help'))
      puts "USAGE: ruby #{$0} [dataset dir [embedding dir [output dir [embedding method [distance type]]]]]"
      puts "Defaults:"
      puts "   dataset dir = #{DEFAULT_DATASET_DIR}"
      puts "   embedding dir = #{DEFAULT_EMBEDDING_DIR}"
      puts "   output dir = #{DEFAULT_OUT_DIR}"
      puts "   embedding method = inferred"
      puts "   distance type = inferred"
      puts ""
      puts "Inferring the embedding method and distance type relies on the emebedding directory"
      puts "being formatted by the evalAll.rb script."
      exit(2)
   end

   if (args.size() > 0)
      datasetDir = args[0]
   end

   if (args.size() > 1)
      embeddingDir = args[1]
   end

   if (args.size() > 2)
      outDir = args[2]
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

   energyMethod = getEnergyMethod(embeddingMethod, distanceType)

   return datasetDir, embeddingDir, outDir, energyMethod
end

# Given an embedding method and distance type, return a proc that will compute the energy.
def getEnergyMethod(embeddingMethod, distanceType)
   distanceProc = nil
   case distanceType
   when L1_DISTANCE_STRING
      distanceProc = proc{|head, tail, relation| Distance.l1(head, tail, relation)}
   when L2_DISTANCE_STRING
      distanceProc = proc{|head, tail, relation| Distance.l2(head, tail, relation)}
   else
      $stderr.puts("Unknown distance type: #{distanceType}")
      exit(3)
   end

   case embeddingMethod
   when EMBEDDING_METHOD_TRANSE
      return proc{|head, tail, relation| TransE.tripleEnergy(distanceProc, head, tail, relation)}
   else
      $stderr.puts("Unknown embedding method: #{embeddingMethod}")
      exit(4)
   end
end

def main(args)
   datasetDir, embeddingDir, outDir, energyMethod = parseArgs(args)

   FileUtils.mkdir_p(outDir)

   copyMappings(datasetDir, outDir)
   convertIds(datasetDir, outDir)
   genTargets(datasetDir, outDir)
   computeEnergies(embeddingDir, outDir, energyMethod)
end

if (__FILE__ == $0)
   main(ARGV)
end
