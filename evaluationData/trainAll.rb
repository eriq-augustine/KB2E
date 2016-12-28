#!/bin/ruby

require 'etc'
require 'fileutils'
require 'open3'

require 'thread/pool'

NUM_THREADS = Etc.nprocessors - 1

SEED = 4

DATA_DIR = File.absolute_path(File.join('..', 'data'))

# We will be working with only one dataset at a time since multiple methods are working with it in parallel.
DATASET_DIR = File.absolute_path(File.join('..', 'FB15k'))

DISTANCE_L1 = 0
DISTANCE_L2 = 0

METHOD_UNIFORM = 0
METHOD_BERNOULLI = 1

NEG_METHOD_NAME = {
   METHOD_UNIFORM => 'unif',
   METHOD_BERNOULLI => 'bern'
}

OUTPUT_DIR = File.absolute_path('.')
EXPERIMENT_DIR = File.absolute_path(File.join('..', 'code'))

EXPERIMENTS = [
   # TransE
   {
      :method => 'TransE',
      :args => {
         'size' => 50,
         'margin' => 1,
         'method' => METHOD_UNIFORM,
         'rate' => 0.01,
         'batches' => 100,
         'epochs' => 1000,
         'distance' => DISTANCE_L1
      }
   },
   {
      :method => 'TransE',
      :args => {
         'size' => 50,
         'margin' => 1,
         'method' => METHOD_BERNOULLI,
         'rate' => 0.01,
         'batches' => 100,
         'epochs' => 1000,
         'distance' => DISTANCE_L1
      }
   },
   # TransH
   {
      :method => 'TransH',
      :args => {
         'size' => 50,
         'margin' => 1,
         'method' => METHOD_UNIFORM,
         'rate' => 0.01,
         'batches' => 100,
         'epochs' => 1000,
         'distance' => DISTANCE_L1
      }
   },
   {
      :method => 'TransH',
      :args => {
         'size' => 50,
         'margin' => 1,
         'method' => METHOD_BERNOULLI,
         'rate' => 0.01,
         'batches' => 100,
         'epochs' => 1000,
         'distance' => DISTANCE_L1
      }
   }
]

# TransR must be after TransH
EXPERIMENTS_2 = [
   {:method => 'TransR', :size => 50, :margin => 1, :negativeMethod => METHOD_UNIFORM, :learningRate => 0.001},
   {:method => 'TransR', :size => 50, :margin => 1, :negativeMethod => METHOD_BERNOULLI, :learningRate => 0.001},

   # TODO(eriq): PTransE Needs a lot of special work, leave out for now.
=begin
   {:method => 'PTransE', :size => 100, :margin => 1, :negativeMethod => METHOD_UNIFORM, :learningRate => 0.001,
      :submethod => 'PTransE_add', :steps => 2},
   {:method => 'PTransE', :size => 100, :margin => 1, :negativeMethod => METHOD_BERNOULLI, :learningRate => 0.001,
      :submethod => 'PTransE_add', :steps => 2},
   {:method => 'PTransE', :size => 100, :margin => 1, :negativeMethod => METHOD_UNIFORM, :learningRate => 0.001,
      :submethod => 'PTransE_mul', :steps => 2},
   {:method => 'PTransE', :size => 100, :margin => 1, :negativeMethod => METHOD_BERNOULLI, :learningRate => 0.001,
      :submethod => 'PTransE_mul', :steps => 2}
=end
]

def getId(experiment)
   method = experiment[:method]
   return "#{method}_[#{experiment[:args].keys.map{|key| "#{key}:#{experiment[:args][key]}"}.join(',')}]"
end

# Get the dir to put the results.
def getOutputDir(experiment)
   return File.absolute_path(File.join(OUTPUT_DIR, getId(experiment)))
end

def prep(experiment)
end

def cleanup(experiment)
end

def getCommand(experiment)
   binName = File.join('.', 'bin', "train#{experiment[:method]}")
   outputDir = getOutputDir(experiment)

   args = [
      "--datadir '#{DATA_DIR}'",
      "--outdir '#{outputDir}'",
      "--seed #{SEED}"
   ]

   experiment[:args].each_pair{|key, value|
      args << "--#{key} '#{value}'"
   }

   return "#{binName} #{args.join(' ')}"
end

def train(experiment)
   prep(experiment)

   outputDir = getOutputDir(experiment)

   stdoutFile = File.absolute_path(File.join(outputDir, 'train.txt'))
   stderrFile = File.absolute_path(File.join(outputDir, 'train.err'))

   # TEST
   puts "Training: #{getId(experiment)}"

   run("mkdir -p '#{outputDir}'")
   run("cd '#{EXPERIMENT_DIR}' && #{getCommand(experiment)}", stdoutFile, stderrFile)

   cleanup(experiment)
end

# Do any global setup like copying data.
def globalSetup(experiments)
   # Copy over the data.
   run("rm -f #{File.join(DATA_DIR, '*')}")
   run("cp #{File.join(DATASET_DIR, '*')} #{DATA_DIR}")

   # Build
   run("cd '#{EXPERIMENT_DIR}' && make clean && make")
end

def run(command, outFile=nil, errFile=nil)
   stdout, stderr, status = Open3.capture3(command)

   if (outFile != nil)
      File.open(outFile, 'w'){|file|
         file.puts(stdout)
      }
   end
   
   if (errFile != nil)
      File.open(errFile, 'w'){|file|
         file.puts(stderr)
      }
   end

   if (status.exitstatus() != 0)
      raise "Failed to run command: [#{command}]. Exited with status: #{status}" + 
            "\n--- Stdout ---\n#{stdout}" +
            "\n--- Stderr ---\n#{stderr}"
   end
end

def trainAll(experiments)
   pool = Thread.pool(NUM_THREADS)

   experiments.each{|experiment|
      outputDir = getOutputDir(experiment)
      if (File.exists?(outputDir))
         next
      end

      pool.process{
         begin
            train(experiment)
         rescue Exception => ex
            puts "Failed to train #{getId(experiment)}"
            puts ex.message()
            puts ex.backtrace()
         end
      }
   }

   pool.shutdown()
end

def main(experiments)
   globalSetup(experiments)
   trainAll(experiments)
end

if (__FILE__ == $0)
   main(EXPERIMENTS)
end
