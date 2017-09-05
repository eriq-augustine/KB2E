#ifndef COMMON_ARGS_H_
#define COMMON_ARGS_H_

#include <string>
#include <vector>

namespace common {

struct EmbeddingArguments {
   std::string dataDir;
   std::string outputDir;
   int embeddingSize;
   double learningRate;
   double margin;
   int method;
   int numBatches;
   int maxEpochs;
   int distanceType;
   std::string seedDataDir;
   int seedMethod;
   unsigned int seed;

   EmbeddingArguments();
   std::string to_string();
};

EmbeddingArguments parseArgs(int argc, char** argv);
void printUsage(char* invokedFile);

}   // namespace common

#endif  // COMMON_ARGS_H_
