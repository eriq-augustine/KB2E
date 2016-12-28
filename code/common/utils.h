#ifndef COMMON_UTILS_H_
#define COMMON_UTILS_H_

#include <string>
#include <vector>

#define PI 3.1415926535897932384626433832795

namespace common {

// normal distribution
double rand(double min, double max);
double normal(double x, double miu, double sigma);
double randn(double miu,double sigma, double min, double max);
double sqr(double x);
double vec_len(std::vector<double> &a);
int argpos(char *flag, bool hasValue, int argc, char **argv);
void norm(std::vector<double> &a, bool ignoreShort = true);
void norm(std::vector<double> &a, std::vector<double> &b, double rate);
int randMax(int x);

bool fileExists(std::string& path);

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
   unsigned int seed;

   EmbeddingArguments();
   std::string to_string();
};

EmbeddingArguments parseArgs(int argc, char** argv);
void printUsage(char* invokedFile);

}   // namespace common

#endif  // COMMON_UTILS_H_
