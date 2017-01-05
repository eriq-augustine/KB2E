# Make a copy of a stndard dataset and corrupt a specified percent of the tuples.

require 'fileutils'

DEFAULT_OUTPUT_BASE_DIR = '.'
DEFAULT_CORRUPT_PERCENT = 0.05
DEFAULT_SEED = Random.new_seed()

ENTITY_ID_FILE = 'entity2id.txt'
RELATION_ID_FILE = 'relation2id.txt'
TEST_FILE = 'test.txt'
TRAIN_FILE = 'train.txt'
VALID_FILE = 'valid.txt'
TO_CORRUPT_FILE = TRAIN_FILE

HEAD = 0
RELATION = 1
TAIL = 2

def corrupt(corruptPercent, seed, outputDir, entities, relations)
   random = Random.new(seed)

   triples = getTriples(outputDir)

   # TODO(eriq)
   # Randomly select |corruptPercent| indecies.
   numCorrupt = triples.size() * corruptPercent
   corruptIndecies = (0...triples.size()).to_a().sample(numCorrupt, random: random)

   corruptIndecies.each{|corruptIndex|
      corruptComponent = random.rand(3)
      if (corruptComponent == HEAD || corruptComponent == TAIL)
         corruptEntity = entities[random.rand(entities.size())]

         if (corruptComponent == HEAD)
            triples[corruptIndex][:head] = corruptEntity
         else
            triples[corruptIndex][:tail] = corruptEntity
         end
      else
         # Corrupt relation
         corruptRelation = relations[random.rand(relations.size())]
         triples[corruptIndex][:relation] = corruptRelation
      end
   }

   writeTriples(outputDir, triples)
end

def writeTriples(outputDir, triples)
   File.open(File.join(outputDir, TO_CORRUPT_FILE), 'w'){|file|
      file.puts(triples.map{|triple| "#{triple[:head]}\t#{triple[:tail]}\t#{triple[:relation]}"}.join("\n"))
   }
end

def getTriples(outputDir)
   triples = []

   File.open(File.join(outputDir, TO_CORRUPT_FILE), 'r'){|file|
      file.each{|line|
         parts = line.strip().split("\t")
         triples << {:head => parts[0], :tail => parts[1], :relation => parts[2]}
      }
   }

   return triples
end

def getEntities(outputDir)
   return getIds(File.join(outputDir, ENTITY_ID_FILE))
end

def getRelations(outputDir)
   return getIds(File.join(outputDir, RELATION_ID_FILE))
end

def getIds(path)
   ids = []

   File.open(path, 'r'){|file|
      file.each{|line|
         parts = line.strip().split("\t")
         ids << parts[0]
      }
   }

   return ids
end

def copyDatafiles(sourceDir, outputDir)
   toCopy = [ENTITY_ID_FILE, RELATION_ID_FILE, TEST_FILE, TRAIN_FILE, VALID_FILE]
   toCopy.each{|filename|
      FileUtils.cp(File.join(sourceDir, filename), File.join(outputDir, filename))
   }
end

def corruptDataset(sourceDir, outputBaseDir, corruptPercent, seed)
   outputDir = File.join(outputBaseDir, "#{File.basename(sourceDir)}_#{("%03d" % (corruptPercent * 100).to_i())}")

   FileUtils.mkdir_p(outputDir)

   copyDatafiles(sourceDir, outputDir)

   entities = getEntities(outputDir)
   relations = getRelations(outputDir)

   corrupt(corruptPercent, seed, outputDir, entities, relations)
end

def main(args)
   if (args.size() < 1 || args.size() > 4)
      puts "USAGE: ruby #{$0} <source dataset dir> [output base dir] [corrupt percent] [seed]"
      puts "Note that the output base dir is the dir that a NEW directory containing the dataset will be created."
      exit(1)
   end

   sourceDir = args[0]
   outputBaseDir = DEFAULT_OUTPUT_BASE_DIR
   corruptPercent = DEFAULT_CORRUPT_PERCENT
   seed = DEFAULT_SEED

   if (args.size() > 1)
      outputBaseDir = args[1]
   end

   if (args.size() > 2)
      corruptPercent = args[2].to_f()

      if (corruptPercent < 0 || corruptPercent > 1)
         puts "Expecting corrupt percentage between 0 and 1."
         exit(2)
      end
   end

   if (args.size() > 3)
      seed = args[3].to_i()
   end

   corruptDataset(sourceDir, outputBaseDir, corruptPercent, seed)
end

if (__FILE__ == $0)
   main(ARGV)
end
