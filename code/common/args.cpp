#include "common/args.h"

#include <sys/stat.h>

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

#include "common/constants.h"
#include "common/utils.h"

namespace common {

EmbeddingArguments::EmbeddingArguments() {
   dataDir = DEFAULT_DATA_DIR;
   outputDir = DEFAULT_OUTPUT_DIR;
   embeddingSize = DEFAULT_EMBEDDING_SIZE;
   learningRate = DEFAULT_LEARNING_RATE;
   margin = DEFAULT_MARGIN;
   method = DEFAULT_METHOD;
   numBatches = DEFAULT_NUM_BATCHES;
   maxEpochs = DEFAULT_MAX_EPOCHS;
   distanceType = DEFAULT_DISTANCE;
   seedDataDir = DEFAULT_SEED_DATA_DIR;
   seedMethod = DEFAULT_SEED_METHOD;
   seed = (unsigned int)time(NULL);
}

std::string EmbeddingArguments::to_string() {
   std::string rtn;

   rtn += "Options: [";
   rtn += std::string(ARG_DATA_DIR) + ": '" + dataDir + "', ";
   rtn += std::string(ARG_OUT_DIR) + ": '" + outputDir + "', ";
   rtn += std::string(ARG_EMBEDDING_SIZE) + ": " + std::to_string(embeddingSize) + ", ";
   rtn += std::string(ARG_LEARNING_RATE) + ": " + std::to_string(learningRate) + ", ";
   rtn += std::string(ARG_MARGIN) + ": " + std::to_string(margin) + ", ";
   rtn += std::string(ARG_METHOD) + ": " + METHOD_TO_STRING(method) + ", ";
   rtn += std::string(ARG_NUM_BATCHES) + ": " + std::to_string(numBatches) + ", ";
   rtn += std::string(ARG_MAX_EPOCHS) + ": " + std::to_string(maxEpochs) + ", ";
   rtn += std::string(ARG_DISTANCE_TYPE) + ": " + std::to_string(distanceType) + ", ";
   rtn += std::string(ARG_SEED_DATA_DIR) + ": '" + seedDataDir + "', ";
   rtn += std::string(ARG_SEED_METHOD) + ": " + std::to_string(seedMethod) + ", ";
   rtn += std::string(ARG_SEED) + ": " + std::to_string(seed) + "]";

   return rtn;
}

EmbeddingArguments parseArgs(int argc, char** argv) {
   int index = argpos((char*)ARG_HELP, false, argc, argv);
   if (index != -1) {
      printUsage((char*)argv[0]);
      exit(0);
   }

   EmbeddingArguments parsedArgs;
   index = argpos((char*)ARG_DATA_DIR, true, argc, argv);
   if (index != -1) {
      parsedArgs.dataDir = argv[index + 1];
   }

   index = argpos((char*)ARG_OUT_DIR, true, argc, argv);
   if (index != -1) {
      parsedArgs.outputDir = argv[index + 1];
   }

   index = argpos((char*)ARG_EMBEDDING_SIZE, true, argc, argv);
   if (index != -1) {
      parsedArgs.embeddingSize = atoi(argv[index + 1]);
   }

   index = argpos((char*)ARG_LEARNING_RATE, true, argc, argv);
   if (index != -1) {
      parsedArgs.learningRate = atof(argv[index + 1]);
   }

   index = argpos((char*)ARG_MARGIN, true, argc, argv);
   if (index != -1) {
      parsedArgs.margin = atof(argv[index + 1]);
   }

   index = argpos((char*)ARG_METHOD, true, argc, argv);
   if (index != -1) {
      parsedArgs.method = atoi(argv[index + 1]);
   }

   index = argpos((char*)ARG_NUM_BATCHES, true, argc, argv);
   if (index != -1) {
      parsedArgs.numBatches = atoi(argv[index + 1]);
   }

   index = argpos((char*)ARG_MAX_EPOCHS, true, argc, argv);
   if (index != -1) {
      parsedArgs.maxEpochs = atoi(argv[index + 1]);
   }

   index = argpos((char*)ARG_DISTANCE_TYPE, true, argc, argv);
   if (index != -1) {
      parsedArgs.distanceType = atoi(argv[index + 1]);
   }

   index = argpos((char*)ARG_SEED_DATA_DIR, true, argc, argv);
   if (index != -1) {
      parsedArgs.seedDataDir = argv[index + 1];
   }

   index = argpos((char*)ARG_SEED_METHOD, true, argc, argv);
   if (index != -1) {
      parsedArgs.seedMethod = atoi(argv[index + 1]);
   }

   index = argpos((char*)ARG_SEED, true, argc, argv);
   if (index != -1) {
      parsedArgs.seed = atoi(argv[index + 1]);
   }

   return parsedArgs;
}

// Param is argv[0].
void printUsage(char* invokedFile) {
   printf("USAGE: %s [option value] ...\n", invokedFile);
   printf("       %s --help\n", invokedFile);
   printf("All options require a value.\n");
   printf("Options:\n");
   printf("   --%s [%s]\n", ARG_DATA_DIR, DEFAULT_DATA_DIR);
   printf("   --%s [%s]\n", ARG_OUT_DIR, DEFAULT_OUTPUT_DIR);
   printf("   --%s [%d]\n", ARG_EMBEDDING_SIZE, DEFAULT_EMBEDDING_SIZE);
   printf("   --%s [%f]\n", ARG_LEARNING_RATE, DEFAULT_LEARNING_RATE);
   printf("   --%s [%f]\n", ARG_MARGIN, DEFAULT_MARGIN);
   printf("   --%s [%d (%s)]\n", ARG_METHOD, DEFAULT_METHOD, METHOD_TO_STRING(DEFAULT_METHOD));
   printf("   --%s [%d]\n", ARG_NUM_BATCHES, DEFAULT_NUM_BATCHES);
   printf("   --%s [%d]\n", ARG_MAX_EPOCHS, DEFAULT_MAX_EPOCHS);
   printf("   --%s [%d]\n", ARG_DISTANCE_TYPE, DEFAULT_DISTANCE);
   printf("   --%s [%s] (TransR only)\n", ARG_SEED_DATA_DIR, DEFAULT_SEED_DATA_DIR);
   printf("   --%s [%d] (TransR only)\n", ARG_SEED_METHOD, DEFAULT_SEED_METHOD);
   printf("   --%s [now]\n", ARG_SEED);
}

}   // namespace common
