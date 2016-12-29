#!/bin/ruby

require 'etc'
require 'fileutils'
require 'open3'

require 'thread/pool'

NUM_THREADS = Etc.nprocessors - 1

SEED = 4

DATA_DIR = File.absolute_path(File.join('..', 'data'))

DATASET_DIR = File.absolute_path(File.join('..', 'datasets'))
FB15K_DATA_DIR = File.absolute_path(File.join(DATASET_DIR, 'FB15k'))

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

COMMAND_TYPE_TRAIN = 'train'
COMMAND_TYPE_EVAL = 'eval'

EXPERIMENTS = [
   # TransE
   {
      :method => 'TransE',
      :data => FB15K_DATA_DIR,
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
      :data => FB15K_DATA_DIR,
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
      :data => FB15K_DATA_DIR,
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
      :data => FB15K_DATA_DIR,
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

def getId(experiment)
   method = experiment[:method]
   data = File.basename(experiment[:data])
   return "#{method}_#{data}_[#{experiment[:args].keys.map{|key| "#{key}:#{experiment[:args][key]}"}.join(',')}]"
end

# Get the dir to put the results.
def getOutputDir(experiment)
   return File.absolute_path(File.join(OUTPUT_DIR, getId(experiment)))
end

def prepTrain(experiment)
end

def cleanupTrain(experiment)
end

def prepEval(experiment)
end

def cleanupEval(experiment)
end

# |type| should be COMMAND_TYPE_TRAIN or COMMAND_TYPE_EVAL
def getCommand(experiment, type)
   binName = File.join('.', 'bin', "#{type}#{experiment[:method]}")
   outputDir = getOutputDir(experiment)

   args = [
      "--datadir '#{experiment[:data]}'",
      "--outdir '#{outputDir}'",
      "--seed #{SEED}"
   ]

   experiment[:args].each_pair{|key, value|
      args << "--#{key} '#{value}'"
   }

   return "#{binName} #{args.join(' ')}"
end

def train(experiment)
   prepTrain(experiment)

   outputDir = getOutputDir(experiment)

   stdoutFile = File.absolute_path(File.join(outputDir, 'train.txt'))
   stderrFile = File.absolute_path(File.join(outputDir, 'train.err'))

   # TEST
   puts "Training: #{getId(experiment)}"

   run("mkdir -p '#{outputDir}'")
   run("cd '#{EXPERIMENT_DIR}' && #{getCommand(experiment, COMMAND_TYPE_TRAIN)}", stdoutFile, stderrFile)

   cleanupTrain(experiment)
end

def evaluate(experiment)
   prepEval(experiment)

   outputDir = getOutputDir(experiment)

   stdoutFile = File.absolute_path(File.join(outputDir, 'eval.txt'))
   stderrFile = File.absolute_path(File.join(outputDir, 'eval.err'))

   # TEST
   puts "Evaluating: #{getId(experiment)}"

   run("cd '#{EXPERIMENT_DIR}' && #{getCommand(experiment, COMMAND_TYPE_EVAL)}", stdoutFile, stderrFile)

   cleanupEval(experiment)
end

# Do any global setup like copying data.
def globalSetup(experiments)
   experiments.each{|experiment|
      if (!File.exists?(experiment[:data]))
         raise "Missing dataset: [#{experiment[:data]}]."
      end
   }

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
            evaluate(experiment)
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
