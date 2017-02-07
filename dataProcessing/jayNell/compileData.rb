require_relative 'base'

require 'date'
require 'fileutils'

# The old Nell data is in many different files, and needs to be compiled into the
# format that the embeddings use.
# All triple rows (both input and output) are in the form: head, tail, relation[, probability] (but tab separated).

# We will only include triples that meet or exceed a minimim onfidence.
DEFAULT_MIN_CONFIDENCE = 0.99

DEFAULT_OUT_DIR = 'generated'

def getTriples(dataDir, minConfidence)
   triples = []
   rejectedCount = 0

   Base::TRIPLE_FILENAMES.each{|filename|
      newTriples, newRejectedCount = Base.readTriples(File.join(dataDir, filename), minConfidence)

      rejectedCount += newRejectedCount
      triples += newTriples
   }

   totalSize = triples.size() + rejectedCount
   dupSize = triples.size()

   puts "Rejected #{rejectedCount} / #{totalSize} triples"

   triples.uniq!()

   puts "Deduped from #{dupSize} -> #{triples.size()}"

   return triples
end

def parseArgs(args)
   if (args.size() < 1 || args.size() > 2 || args.map{|arg| arg.downcase().strip().sub(/^-+/, '')}.include?('help'))
      puts "USAGE: ruby #{$0} <data dir> [minimum confidence]"
      puts "minimum confidence -- Default: #{DEFAULT_MIN_CONFIDENCE}"
      exit(1)
   end

   dataDir = args[0]
   minConfidence = DEFAULT_MIN_CONFIDENCE

   if (args.size() == 2)
      minConfidence = args[1].to_f()

      if (minConfidence < 0 || minConfidence > 1)
         puts "Minimum confidence must be between 0 and 1."
         exit(2)
      end
   end

   return dataDir, minConfidence
end

def compileData(dataDir, minConfidence)
   triples = getTriples(dataDir, minConfidence)

   outDir = File.join(DEFAULT_OUT_DIR, "NELL_JAY_#{DateTime.now().strftime('%Y%m%d%H%M')}")
   FileUtils.mkdir_p(outDir)

   Base.writeEntities(File.join(outDir, Base::ENTITY_ID_FILENAME), triples)
   Base.writeRelations(File.join(outDir, Base::RELATION_ID_FILENAME), triples)

   Base.writeTriples(File.join(outDir, Base::TRAIN_FILENAME), triples)

   # We won't actually use test or train files, but make empty ones anyways.
   FileUtils.touch(File.join(outDir, Base::TEST_FILENAME))
   FileUtils.touch(File.join(outDir, Base::VALID_FILENAME))
end

def main(args)
   dataDir, minConfidence = parseArgs(args)
   compileData(dataDir, minConfidence)
end

if (__FILE__ == $0)
   main(ARGV)
end
