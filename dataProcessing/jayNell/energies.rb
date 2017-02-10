require_relative 'base'
require_relative '../embeddings'

# gem install thread
require 'thread/pool'

module Energies
   NUM_THREADS = Etc.nprocessors - 1
   MIN_WORK_PER_THREAD = 100

   def Energies.computeEnergies(triples, entityMapping, relationMapping, entityEmbeddings, relationEmbeddings, energyMethod, skipBadEnergies = false)
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

               if (!skipBadEnergies || ok)
                  lock.synchronize {
                     energies[id] = energy
                  }
               end
            }
         }
      }

      pool.wait(:done)

      pool.shutdown()

      # Remove rejected energies.
      energies.delete_if{|key, value| value == -1}

      return energies
   end

   # TODO(eriq): This is not an energy specific method.
   #  This should be in a more general place.
   #  Maybe have a param to Embeddings.loadMappings that will convert the keys to ints.
   # Load the entity/relation mappings for the embeddings, and convert the key and value types to ints.
   def Energies.loadMappings(datasetDir)
      entityMapping, relationMapping = Embeddings.loadMappings(datasetDir)

      entityMapping = entityMapping.to_a().map{|key, val| [key.to_i(), val.to_i()]}.to_h()
      relationMapping = relationMapping.to_a().map{|key, val| [key.to_i(), val.to_i()]}.to_h()

      return entityMapping, relationMapping
   end

   # TODO(eriq): This belongs in a more general lib.
   def Energies.getTriples(sourceDir)
      triples = []

      Base::TRIPLE_FILENAMES.each{|filename|
         newTriples, newRejectedCount = Base.readTriples(File.join(sourceDir, filename), -1)
         triples += newTriples
      }
      triples.uniq!()

      return triples
   end
end
