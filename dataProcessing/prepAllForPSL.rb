#!/bin/ruby

# A convenience script for prep all the evaluation data for PSL.

require 'etc'

# gem install thread
require 'thread/pool'

require './prepForPSL.rb'

# NUM_JOBS = Etc.nprocessors - 1
NUM_JOBS = 1

DEFAULT_DATA_DIR = File.join('..', 'evaluationData', 'results')

def prepAll(dataDir)
   pool = Thread.pool(NUM_JOBS)

   Dir.entries(dataDir).sort().each{|dir|
      if (['.', '..'].include?(dir))
         next
      end

      path = File.join(dataDir, dir)

      pool.process{
         begin
            puts "Prepping: #{path}"
            prepForPSL([path])
         rescue Exception => ex
            puts "Failed to prep #{path}"
            puts ex.message()
            puts ex.backtrace()
         end
      }
   }

   pool.wait()
   pool.shutdown()
end

def main(args)
   if (args.size() > 1)
      puts "USAGE: ruby #{$0} [data dir]"
      exit(1)
   end

   dataDir = DEFAULT_DATA_DIR
   if (args.size() == 1)
      dataDir = args[0]
   end

   prepAll(dataDir)
end

if (__FILE__ == $0)
   main(ARGV)
end
