#!/bin/ruby

require 'date'
require 'fileutils'

require 'pg'

OUT_DIR = 'generated'
OUT_BASENAME = 'NELL'
DB_NAME = 'nell'

# How much of the data to use for a training set.
TRAINING_PERCENT = 0.90

DEFAULT_MIN_PROBABILITY = 0.95
DEFAULT_MAX_PROBABILITY = 1.00
DEFAULT_MIN_ENTITY_MENTIONS = 20
DEFAULT_MIN_RELATION_MENTIONS = 5

# About the same as Freebase.
DEFAULT_MAX_TRIPLES = 500000

ENTITY_ID_FILENAME = 'entity2id.txt'
RELATION_ID_FILENAME = 'relation2id.txt'
TEST_FILENAME = 'test.txt'
TRAIN_FILENAME = 'train.txt'
VALID_FILENAME = 'valid.txt'

def formatDatasetName(suffix, minProbability, maxProbability, minEntityMentions, minRelationMentions, maxTriples)
   return "#{OUT_BASENAME}_#{"%03d" % (minProbability * 100)}_#{"%03d" % (maxProbability * 100)}_[#{"%03d" % minEntityMentions},#{"%03d" % minRelationMentions},#{maxTriples}]_#{suffix}"
end

def fetchTriples(minProbability, maxProbability, minEntityMentions, minRelationMentions, maxTriples)
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
         T.probability BETWEEN #{minProbability} AND #{maxProbability}
         AND HEC.entityCount >= #{minEntityMentions}
         AND RC.relationCount >= #{minRelationMentions}
         AND TEC.entityCount >= #{minEntityMentions}
      LIMIT #{maxTriples}
   "

	result = conn.exec(query).values()
   conn.close()

   return result
end

def writeEntities(path, triples)
   entities = []
   entities += triples.map{|triple| triple[0]}
   entities += triples.map{|triple| triple[2]}
   entities.uniq!

   File.open(path, 'w'){|file|
      file.puts(entities.map.with_index{|entity, index| "#{entity}\t#{index}"}.join("\n"))
   }
end

def writeRelations(path, triples)
   relations = triples.map{|triple| triple[1]}
   relations.uniq!

   File.open(path, 'w'){|file|
      file.puts(relations.map.with_index{|relation, index| "#{relation}\t#{index}"}.join("\n"))
   }
end

def writeTriples(path, triples)
    File.open(path, 'w'){|file|
      # Head, Tail, Relation
      file.puts(triples.map{|triple| "#{triple[0]}\t#{triple[2]}\t#{triple[1]}"}.join("\n"))
    }
end

def printUsage()
   puts "USAGE: ruby #{$0} [min probability [max probability [min entity mentions [min relation mentions [max triples [suffix]]]]]]"
   puts "Defaults:"
   puts "   min probability = #{DEFAULT_MIN_PROBABILITY}"
   puts "   max probability = #{DEFAULT_MAX_PROBABILITY}"
   puts "   min entity mentions = #{DEFAULT_MIN_ENTITY_MENTIONS}"
   puts "   min relation mentions = #{DEFAULT_MIN_RELATION_MENTIONS}"
   puts "   max triples = #{DEFAULT_MAX_TRIPLES}"
   puts "   suffix = now"
end

def parseArgs(args)
   if (args.size() > 6 || args.map{|arg| arg.downcase().gsub('-', '')}.include?('help'))
      printUsage()
      exit(2)
   end

   minProbability = DEFAULT_MIN_PROBABILITY
   maxProbability = DEFAULT_MAX_PROBABILITY
   minEntityMentions = DEFAULT_MIN_ENTITY_MENTIONS
   minRelationMentions = DEFAULT_MIN_RELATION_MENTIONS
   maxTriples = DEFAULT_MAX_TRIPLES
   suffix = DateTime.now().strftime('%Y%m%d%H%M')

   if (args.size() > 0)
      minProbability = args[0].to_f()
   end

   if (args.size() > 1)
      maxProbability = args[1].to_f()
   end

   if (args.size() > 2)
      minEntityMentions = args[2].to_i()
   end

   if (args.size() > 3)
      minRelationMentions = args[3].to_i()
   end

   if (args.size() > 4)
      maxTriples = args[4].to_i()
   end

   if (args.size() > 5)
      suffix = args[5]
   end

   if (minProbability < 0 || minProbability > 1 || maxProbability < 0 || maxProbability > 1)
      puts "Probabilities should be between 0 and 1 inclusive."
      exit(3)
   end

   if (maxTriples < 0)
      puts "Max Triples needs to be non-negative."
      exit(4)
   end

   if (minEntityMentions < 0 || minRelationMentions < 0)
      puts "Entity/Relation mentions need to be non-negative."
      exit(5)
   end

   return minProbability, maxProbability, minEntityMentions, minRelationMentions, maxTriples, suffix
end

def main(args)
   minProbability, maxProbability, minEntityMentions, minRelationMentions, maxTriples, suffix = parseArgs(args)

   triples = fetchTriples(minProbability, maxProbability, minEntityMentions, minRelationMentions, maxTriples)

   datasetDir = File.join(OUT_DIR, formatDatasetName(suffix, minProbability, maxProbability, minEntityMentions, minRelationMentions, maxTriples))
   FileUtils.mkdir_p(datasetDir)

   writeEntities(File.join(datasetDir, ENTITY_ID_FILENAME), triples)
   writeRelations(File.join(datasetDir, RELATION_ID_FILENAME), triples)

   # TODO(eriq): We probably need smarter splitting?
   trainingSize = (triples.size() * TRAINING_PERCENT).to_i()

   # Both test and valid sets will get this count.
   # The rounding error on odd is a non-issue. The valid will just have one less.
   testSize = ((triples.size() - trainingSize) / 2 + 0.5).to_i()

   triples.shuffle!
   trainingSet = triples.slice(0, trainingSize)
   testSet = triples.slice(trainingSize, testSize)
   validSet = triples.slice(trainingSize + testSize, testSize)

   writeTriples(File.join(datasetDir, TRAIN_FILENAME), trainingSet)
   writeTriples(File.join(datasetDir, TEST_FILENAME), testSet)
   writeTriples(File.join(datasetDir, VALID_FILENAME), validSet)
end

if (__FILE__ == $0)
   main(ARGV)
end


