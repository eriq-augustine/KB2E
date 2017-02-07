# Moule for working with embedding files.

require_relative 'transE'
require_relative 'transH'

module Embeddings
   WEIGHT_EMBEDDING_BASENAME = 'weights'
   ENTITY_EMBEDDING_BASENAME = 'entity2vec'
   RELATION_EMBEDDING_BASENAME = 'relation2vec'

   EMBEDDING_UNIF_SUFFIX = 'unif'
   EMBEDDING_BERN_SUFFIX = 'bern'

   ENTITY_ID_FILE = 'entity2id.txt'
   RELATION_ID_FILE = 'relation2id.txt'

   L1_DISTANCE_STRING = 'L1'
   L2_DISTANCE_STRING = 'L2'

   EMBEDDING_METHOD_TRANSE = 'TransE'
   EMBEDDING_METHOD_TRANSH = 'TransH'

   def Embeddings.loadMapping(path)
      mapping = {}
      File.open(path, 'r'){|file|
         file.each{|line|
            parts = line.split("\t").map{|part| part.strip()}
            mapping[parts[0]] = parts[1]
         }
      }
      return mapping
   end

   def Embeddings.loadMappings(datasetDir)
      entityMapping = Embeddings.loadMapping(File.join(datasetDir, ENTITY_ID_FILE))
      relationMapping = Embeddings.loadMapping(File.join(datasetDir, RELATION_ID_FILE))
      return entityMapping, relationMapping
   end

   # Note: The if of the entity/relation is the line index in the file.
   def Embeddings.loadEmbeddingFile(path)
      embeddings = []

      File.open(path, 'r'){|file|
         file.each{|line|
            embeddings << line.split("\t").map{|part| part.strip().to_f()}
         }
      }

      return embeddings
   end

   def Embeddings.loadWeights(embeddingDir)
      # Check if we are workig with unif or bern.
      if (File.exists?(File.join(embeddingDir, WEIGHT_EMBEDDING_BASENAME + '.' + EMBEDDING_UNIF_SUFFIX)))
         # Unif
         return Embeddings.loadEmbeddingFile(File.join(embeddingDir, WEIGHT_EMBEDDING_BASENAME + '.' + EMBEDDING_UNIF_SUFFIX))
      else
         # Bern
         return Embeddings.loadEmbeddingFile(File.join(embeddingDir, WEIGHT_EMBEDDING_BASENAME + '.' + EMBEDDING_BERN_SUFFIX))
      end
   end

   def Embeddings.loadEmbeddings(embeddingDir)
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

      entityEmbeddings = Embeddings.loadEmbeddingFile(entityPath)
      relationEmbeddings = Embeddings.loadEmbeddingFile(relationPath)

      return entityEmbeddings, relationEmbeddings
   end

   # Given an embedding method and distance type, return a proc that will compute the energy.
   def Embeddings.getEnergyMethod(embeddingMethod, distanceType, embeddingDir)
      if (![L1_DISTANCE_STRING, L2_DISTANCE_STRING].include?(distanceType))
         raise("Unknown distance type: #{distanceType}")
      end

      case embeddingMethod
      when EMBEDDING_METHOD_TRANSE
         return proc{|head, tail, relation, headId, tailId, relationId|
            TransE.tripleEnergy(distanceType, head, tail, relation)
         }
      when EMBEDDING_METHOD_TRANSH
         transHWeights = Embeddings.loadWeights(embeddingDir)
         return proc{|head, tail, relation, headId, tailId, relationId|
            TransH.tripleEnergy(head, tail, relation, transHWeights[relationId])
         }
      else
         $stderr.puts("Unknown embedding method: #{embeddingMethod}")
         exit(4)
      end
   end
end
