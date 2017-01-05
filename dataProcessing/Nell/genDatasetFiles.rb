#!/bin/ruby

require 'fileutils'

require 'pg'

OUT_DIR = 'NELL'
DB_NAME = 'nell'

# How much of the data to use for a training set.
TRAINING_PERCENT = 0.90

MIN_ENTITY_MENTIONS = 20
MIN_RELATION_MENTIONS = 5

ENTITY_ID_FILE = File.join(OUT_DIR, 'entity2id.txt')
RELATION_ID_FILE = File.join(OUT_DIR, 'relation2id.txt')
TEST_FILE = File.join(OUT_DIR, 'test.txt')
TRAIN_FILE = File.join(OUT_DIR, 'train.txt')
VALID_FILE = File.join(OUT_DIR, 'valid.txt')

def fetchTriples()
   conn = PG::Connection.new(:host => 'localhost', :dbname => DB_NAME)

   query = "
      SELECT
         T.head,
         T.relation,
         T.tail
      FROM
         Triples T
         JOIN CandidateTriples CT ON CT.tripleId = T.id
         JOIN EntityCounts HEC ON HEC.entityId = T.head
         JOIN RelationCounts RC ON RC.relationId = T.relation
         JOIN EntityCounts TEC ON TEC.entityId = T.tail
      WHERE
         HEC.entityCount >= #{MIN_ENTITY_MENTIONS}
         AND RC.relationCount >= #{MIN_RELATION_MENTIONS}
         AND TEC.entityCount >= #{MIN_ENTITY_MENTIONS}
   "

	result = conn.exec(query)
   return result.values()
end

def writeEntities(triples)
   entities = []
   entities += triples.map{|triple| triple[0]}
   entities += triples.map{|triple| triple[2]}
   entities.uniq!

   File.open(ENTITY_ID_FILE, 'w'){|file|
      file.puts(entities.map.with_index{|entity, index| "#{entity}\t#{index}"}.join("\n"))
   }
end

def writeRelations(triples)
   relations = triples.map{|triple| triple[1]}
   relations.uniq!

   File.open(RELATION_ID_FILE, 'w'){|file|
      file.puts(relations.map.with_index{|relation, index| "#{relation}\t#{index}"}.join("\n"))
   }
end

def writeTriples(path, triples)
    File.open(path, 'w'){|file|
      # Head, Tail, Relation
      file.puts(triples.map{|triple| "#{triple[0]}\t#{triple[2]}\t#{triple[1]}"}.join("\n"))
    }
end

def main(args)
   FileUtils.mkdir_p(OUT_DIR)

   triples = fetchTriples

   writeEntities(triples)
   writeRelations(triples)

   # TODO(eriq): We probably need smarter splitting?
   trainingSize = (triples.size() * TRAINING_PERCENT).to_i()

   # Both test and valid sets will get this count.
   # The rounding error on odd is a non-issue. The valid will just have one less.
   testSize = ((triples.size() - trainingSize) / 2 + 0.5).to_i()

   triples.shuffle!
   trainingSet = triples.slice(0, trainingSize)
   testSet = triples.slice(trainingSize, testSize)
   validSet = triples.slice(trainingSize + testSize, testSize)

   writeTriples(TRAIN_FILE, trainingSet)
   writeTriples(TEST_FILE, testSet)
   writeTriples(VALID_FILE, validSet)
end

if (__FILE__ == $0)
   main(ARGV)
end


