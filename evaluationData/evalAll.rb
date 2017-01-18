#!/bin/ruby

require 'etc'
require 'fileutils'
require 'open3'

# gem install thread
require 'thread/pool'

NUM_THREADS = Etc.nprocessors - 1

SEED = 4

DATA_DIR = File.absolute_path(File.join('..', 'data'))

DATASET_DIR = File.absolute_path(File.join('..', 'datasets'))

FB15K_DATA_DIR = File.absolute_path(File.join(DATASET_DIR, 'FB15k'))
FB15K_005_DATA_DIR = File.absolute_path(File.join(DATASET_DIR, 'FB15k_005'))
FB15K_010_DATA_DIR = File.absolute_path(File.join(DATASET_DIR, 'FB15k_010'))
FB15K_050_DATA_DIR = File.absolute_path(File.join(DATASET_DIR, 'FB15k_050'))

NELL_DATA_DIR = File.absolute_path(File.join(DATASET_DIR, 'NELL_95'))

UNCERTIAN_NELL_DIRS = [
	'NELL_050_080_[005,005]_201701141040',
	'NELL_050_080_[020,005]_201701141040',
	'NELL_050_080_[020,020]_201701141040',
	'NELL_050_080_[050,050]_201701141040',
	'NELL_050_080_[100,100]_201701141040',
	'NELL_050_100_[005,005]_201701141040',
	'NELL_050_100_[020,005]_201701141040',
	'NELL_050_100_[020,020]_201701141040',
	'NELL_050_100_[050,050]_201701141040',
	'NELL_050_100_[100,100]_201701141040',
	'NELL_080_090_[005,005]_201701141040',
	'NELL_080_090_[020,005]_201701141040',
	'NELL_080_090_[020,020]_201701141040',
	'NELL_080_090_[050,050]_201701141040',
	'NELL_080_090_[100,100]_201701141040',
	'NELL_090_100_[005,005]_201701141040',
	'NELL_090_100_[020,005]_201701141040',
	'NELL_090_100_[020,020]_201701141040',
	'NELL_090_100_[050,050]_201701141040',
	'NELL_090_100_[100,100]_201701141040',
	'NELL_095_100_[005,005]_201701141040',
	'NELL_095_100_[020,005]_201701141040',
	'NELL_095_100_[020,020]_201701141040',
	'NELL_095_100_[050,050]_201701141040',
	'NELL_095_100_[100,100]_201701141040',
	'NELL_100_100_[005,005]_201701141040',
	'NELL_100_100_[020,005]_201701141040',
	'NELL_100_100_[020,020]_201701141040',
	'NELL_100_100_[050,050]_201701141040',
	'NELL_100_100_[100,100]_201701141040'
].map{|basename| File.absolute_path(File.join(DATASET_DIR, basename))}

DISTANCE_L1 = 0
DISTANCE_L2 = 1

METHOD_UNIFORM = 0
METHOD_BERNOULLI = 1

NEG_METHOD_NAME = {
   METHOD_UNIFORM => 'unif',
   METHOD_BERNOULLI => 'bern'
}

OUTPUT_DIR = File.absolute_path('.', 'results')
EXPERIMENT_DIR = File.absolute_path(File.join('..', 'code'))

COMMAND_TYPE_TRAIN = 'train'
COMMAND_TYPE_EVAL = 'eval'

TRANSE_EXPERIMENTS = {
   :method => 'TransE',
   :data => [FB15K_DATA_DIR, FB15K_005_DATA_DIR, FB15K_010_DATA_DIR, FB15K_050_DATA_DIR, NELL_DATA_DIR] + UNCERTIAN_NELL_DIRS,
   :args => {
      :size => [50, 100],
      :method => [METHOD_UNIFORM, METHOD_BERNOULLI],
      :distance => [DISTANCE_L1, DISTANCE_L2]
   }
}

TRANSH_EXPERIMENTS = {
   :method => 'TransH',
   :data => [FB15K_DATA_DIR, FB15K_005_DATA_DIR, FB15K_010_DATA_DIR, FB15K_050_DATA_DIR, NELL_DATA_DIR] + UNCERTIAN_NELL_DIRS,
   :args => {
      :size => [50, 100],
      :method => [METHOD_UNIFORM, METHOD_BERNOULLI],
      :distance => [DISTANCE_L1]
   }
}

# Make sure the core settings mirror TRANSE since that is the seed data.
TRANSR_EXPERIMENTS = {
   :method => 'TransR',
   :data => [FB15K_DATA_DIR, FB15K_005_DATA_DIR, FB15K_010_DATA_DIR, FB15K_050_DATA_DIR, NELL_DATA_DIR],
   :args => {
      :size => [50, 100],
      :method => [METHOD_UNIFORM, METHOD_BERNOULLI],
      :distance => [DISTANCE_L1, DISTANCE_L2]
   }
}

# TransR needs some additional params.
def buildTransRExperiments(experimentsDefinition)
   experiments = buildExperiments(experimentsDefinition)

   experiments.each{|experiment|
      # TransE is the seed data, so grab the data from there.
      seeDataDir = getOutputDir(experiment).sub('TransR', 'TransE')
      experiment[:args]['seeddatadir'] = seeDataDir

      # TODO(eriq): We are actually missing a set of experiments here.
      # The seed method and outer method are actually independent.
      experiment[:args]['seedmethod'] = experiment[:args]['method']
   }

   return experiments
end

# Take a condensed definition of some experiments and expand it out.
# TODO(eriq): This is pretty hacky and not robust at all.
def buildExperiments(experimentsDefinition)
   experiments = []

   experimentsDefinition[:data].each{|dataset|
      experimentsDefinition[:args][:size].each{|embeddingSize|
         experimentsDefinition[:args][:method].each{|method|
            experimentsDefinition[:args][:distance].each{|distance|
               experiments << {
                  :method => experimentsDefinition[:method],
                  :data => dataset,
                  :args => {
                     'size' => embeddingSize,
                     'margin' => 1,
                     'method' => method,
                     'rate' => 0.01,
                     'batches' => 100,
                     'epochs' => 1000,
                     'distance' => distance
                  }
               }
            }
         }
      }
   }

   return experiments
end

def getId(experiment)
   method = experiment[:method]
   data = File.basename(experiment[:data])

   # Make a copy of the args, since we may need to override some for id purposes.
   args = Hash.new().merge(experiment[:args])

   if (args.include?('seeddatadir'))
      args['seeddatadir'] = File.basename(args['seeddatadir'])
   end

   argString = args.keys.map{|key| "#{key}:#{args[key]}"}.join(',')
   return "#{method}_#{data}_[#{argString}]"
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

def main(args)
   experiments = buildExperiments(TRANSE_EXPERIMENTS) + buildExperiments(TRANSH_EXPERIMENTS)

   # Some methods require data from other experiments and must be run after.
   experiments2 = buildTransRExperiments(TRANSR_EXPERIMENTS)

   globalSetup(experiments)

   trainAll(experiments)
   # trainAll(experiments2)
end

if (__FILE__ == $0)
   main(ARGV)
end
