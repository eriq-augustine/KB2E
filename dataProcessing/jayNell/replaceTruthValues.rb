require_relative 'base'
require_relative '../embeddings'

require 'date'
require 'fileutils'

# gem install thread
require 'thread/pool'

# Given a data directory, copy it and replace the relation truth values with
# ones computed from embedding energies.

NUM_THREADS = Etc.nprocessors - 1
MIN_WORK_PER_THREAD = 100
PAGE_SIZE = 1000

DEFAULT_OUT_DIR = 'replaced'
DATASETS_BASEDIR = File.join('..', '..', 'datasets')

ENERGY_FILE = 'energies.txt'

L1_DISTANCE = 0
L2_DISTANCE = 1

def loadEnergiesFromFile(path)
   energies = {}

   File.open(path, 'r'){|file|
      file.each{|line|
         parts = line.split("\t").map{|part| part.strip()}
         energies[parts[0...3].join(':')] = parts[3].to_f()
      }
   }

   return energies
end

def writeEnergies(path, energies)
   File.open(path, 'w'){|file|
      energies.each_slice(PAGE_SIZE){|page|
         file.puts(page.map{|energyId, energy| "#{energyId.gsub(':', "\t")}\t#{energy}"}.join("\n"))
      }
   }
end

def computeEnergies(triples, entityMapping, relationMapping, entityEmbeddings, relationEmbeddings, energyMethod)
   energies = {}

   pool = Thread.pool(NUM_THREADS)
   lock = Mutex.new()

   triples.each_slice([triples.size() / NUM_THREADS + 1, MIN_WORK_PER_THREAD].max()){|threadTriples|
      pool.process{
         threadTriples.each{|triple|
            id = triple.join(':')

            skip = false
            lock.synchronize {
               if (energies.has_key?(id))
                  skip = true
               else
                  # Mark the key so others don't try to take it mid-computation.
                  energies[id] = -1
               end
            }

            if (skip)
               next
            end

            # It is possible for the entity/relation to not exist if it got filtered
            # out for having too low a confidence score.
            # For these, just leave them out of the energy mapping.
            if (!entityMapping.has_key?(triple[0]) || !entityMapping.has_key?(triple[1]) || !relationMapping.has_key?(triple[2]))
               next
            end

            ok, energy = energyMethod.call(
               entityEmbeddings[entityMapping[triple[0]]],
               entityEmbeddings[entityMapping[triple[1]]],
               relationEmbeddings[relationMapping[triple[2]]],
               entityMapping[triple[0]],
               entityMapping[triple[1]],
               relationMapping[triple[2]]
            )

            lock.synchronize {
               energies[id] = energy
            }
         }
      }
   }

   pool.wait(:done)

   pool.shutdown()

   # Remove rejected energies.
   energies.delete_if{|key, value| value == -1}

   return energies
end

# Returns: {'head:tail:relation' => energy, ...}
def getEnergies(sourceDir, datasetDir, embeddingDir, embeddingMethod, distanceType)
   energyPath = File.join(datasetDir, ENERGY_FILE)
   if (File.exists?(energyPath))
      return loadEnergiesFromFile(energyPath)
   end

   triples = getTriples(sourceDir)

   # Note that the embeddings are indexed by the value in the mappings (eg. entity2id.txt).
   entityMapping, relationMapping = loadMappings(datasetDir)
   entityEmbeddings, relationEmbeddings = Embeddings.loadEmbeddings(embeddingDir)

   energyMethod = Embeddings.getEnergyMethod(embeddingMethod, distanceType, embeddingDir)

   energies = computeEnergies(triples, entityMapping, relationMapping, entityEmbeddings, relationEmbeddings, energyMethod)
   writeEnergies(energyPath, energies)

   return energies
end

# Load the entity/relation mappings for the embeddings, and convert the key and value types to ints.
def loadMappings(datasetDir)
   entityMapping, relationMapping = Embeddings.loadMappings(datasetDir)

   entityMapping = entityMapping.to_a().map{|key, val| [key.to_i(), val.to_i()]}.to_h()
   relationMapping = relationMapping.to_a().map{|key, val| [key.to_i(), val.to_i()]}.to_h()

   return entityMapping, relationMapping
end

def getTriples(sourceDir)
   triples = []

   Base::TRIPLE_FILENAMES.each{|filename|
      newTriples, newRejectedCount = Base.readTriples(File.join(sourceDir, filename), -1)
      triples += newTriples
   }
   triples.uniq!()

   return triples
end

def normalizeEnergies(energies)
   minEnergy, maxEnergy = energies.values().minmax()

   energies.keys().each{|key|
      energies[key] = 1.0 - ((energies[key] - minEnergy) / (maxEnergy - minEnergy))
   }
end

def replaceEnergies(sourceDir, outDir, energies)
   FileUtils.cp_r(sourceDir, outDir)

   Base::TRIPLE_FILENAMES.each{|filename|
      triples = []

      File.open(File.join(outDir, filename), 'r'){|file|
         file.each{|line|
            parts = line.split("\t").map{|part| part.strip()}
            triples << parts[0...3].map{|part| part.to_i()} + [parts[3].to_f()]
         }
      }

      triples.each_index{|i|
         id = triples[i][0...3].join(':')

         if (energies.has_key?(id))
            triples[i][3] = energies[id]
         end
      }

      File.open(File.join(outDir, filename), 'w'){|file|
         triples.each_slice(PAGE_SIZE){|page|
            file.puts(page.map{|triple| triple.join("\t")}.join("\n"))
         }
      }
   }
end

def main(args)
   sourceDir, datasetDir, embeddingDir, outDir, embeddingMethod, distanceType = parseArgs(args)

   FileUtils.mkdir_p(File.absolute_path(File.join(outDir, '..')))

   energies = getEnergies(sourceDir, datasetDir, embeddingDir, embeddingMethod, distanceType)
   normalizeEnergies(energies)
   replaceEnergies(sourceDir, outDir, energies)
end

def parseArgs(args)
   sourceDir = nil
   embeddingDir = nil
   outDir = nil
   datasetDir = nil
   embeddingMethod = nil
   distanceType = nil

   if (args.size() < 1 || args.size() > 6 || args.map{|arg| arg.downcase().gsub('-', '')}.include?('help'))
      puts "USAGE: ruby #{$0} <source dir> <embedding dir> [output dir [dataset dir [embedding method [distance type]]]]"
      puts "Defaults:"
      puts "   output dir = inferred"
      puts "   dataset dir = inferred"
      puts "   embedding method = inferred"
      puts "   distance type = inferred"
      puts ""
      puts "All the inferred aguments relies on the source and emebedding directory"
      puts "being formatted by the evalAll.rb script."
      puts "The directory that the inferred output directory will be put in is: #{DEFAULT_OUT_DIR}."
      exit(2)
   end

   if (args.size() > 0)
      sourceDir = args[0]
   end

   if (args.size() > 1)
      embeddingDir = args[1]
   end

   if (args.size() > 2)
      outDir = args[2]
   else
      outDir = File.join(DEFAULT_OUT_DIR, File.basename(sourceDir))
   end

   if (args.size() > 3)
      datasetDir = args[3]
   else
      dataset = File.basename(embeddingDir).match(/^[^_]+_(\S+)_\[size:/)[1]
      datasetDir = File.join(DATASETS_BASEDIR, File.join(dataset))
   end

   if (args.size() > 4)
      embeddingMethod = args[4]
   else
      embeddingMethod = File.basename(embeddingDir).match(/^([^_]+)_/)[1]
   end

   if (args.size() > 5)
      distanceType = args[5]
   else
      # TODO(eriq): This may be a little off for TransR.
      if (embeddingDir.include?("distance:#{L1_DISTANCE}"))
         distanceType = Embeddings::L1_DISTANCE_STRING
      elsif (embeddingDir.include?("distance:#{L2_DISTANCE}"))
         distanceType = Embeddings::L2_DISTANCE_STRING
      end
   end

   return sourceDir, datasetDir, embeddingDir, outDir, embeddingMethod, distanceType
end

if (__FILE__ == $0)
   main(ARGV)
end
