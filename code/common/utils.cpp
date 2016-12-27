#include "common/utils.h"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>

#include "common/constants.h"

namespace common {

// normal distribution
double rand(double min, double max) {
    return min + (max - min) * std::rand() / (RAND_MAX + 1.0);
}

double normal(double x, double miu, double sigma) {
    return 1.0 / std::sqrt(2 * PI) / sigma * std::exp(-1 * sqr(x - miu) / (2 * sqr(sigma)));
}

double randn(double miu,double sigma, double min, double max) {
    double x;
    double y;
    double dScope;

    do {
        x = rand(min, max);
        y = normal(x, miu, sigma);
        dScope = rand(0.0, normal(miu, miu, sigma));
    } while(dScope > y);

    return x;
}

double sqr(double x) {
    return x * x;
}

double vec_len(std::vector<double> &a) {
    double res = 0;
    for (int i = 0; i < a.size(); i++) {
        res += sqr(a[i]);
    }

    return std::sqrt(res);
}

// Check if the flag (- or -- version) exists in the args.
// If so, return its index, otherwise -1.
int argpos(char* flag, int argc, char** argv) {
   for (int i = 1; i < argc; i++) {
      if (!std::strcmp((std::string("-") + flag).c_str(), argv[i]) || !std::strcmp((std::string("--") + flag).c_str(), argv[i])) {
         if (i == argc - 1) {
            printf("Argument missing for %s\n", flag);
            exit(1);
         }

         return i;
      }
   }

   return -1;
}

void norm(std::vector<double> &a) {
   double len = vec_len(a);
   if (len > 1) {
      for (int i = 0; i < a.size(); i++) {
         a[i] /= len;
      }
   }
}

void norm(std::vector<double> &a, std::vector<double> &b, double rate) {
    assert(a.size() == b.size());

    norm(b);
    double sum = 0;

    while (true) {
        for (int i = 0; i < b.size(); i++) {
            sum += sqr(b[i]);
        }
        sum = std::sqrt(sum);

        for (int i = 0; i < b.size(); i++) {
            b[i] /= sum;
        }

        double x = 0;
        for (int i = 0; i < a.size(); i++) {
            x += b[i] * a[i];
        }

        if (x > 0.1) {
            for (int i = 0; i < a.size(); i++) {
                a[i] -= rate * b[i];
                b[i] -= rate * a[i];
            }
        } else {
            break;
        }
    }

    norm(b);
}

int randMax(int x) {
   int res = (std::rand() * std::rand()) % x;
   while (res < 0) {
      res += x;
   }

   return res;
}

TrainerArguments::TrainerArguments() {
   dataDir = DEFAULT_DATA_DIR;
   outputDir = DEFAULT_OUTPUT_DIR;
   embeddingSize = DEFAULT_EMBEDDING_SIZE;
   learningRate = DEFAULT_LEARNING_RATE;
   margin = DEFAULT_MARGIN;
   method = DEFAULT_METHOD;
   numBatches = DEFAULT_NUM_BATCHES;
   maxEpochs = DEFAULT_MAX_EPOCHS;
   distanceType = DEFAULT_DISTANCE;
}

std::string TrainerArguments::to_string() {
   std::string rtn;

   rtn += "Options: [";
   rtn += std::string(ARG_DATA_DIR) + ": " + dataDir + ", ";
   rtn += std::string(ARG_OUT_DIR) + ": " + outputDir + ", ";
   rtn += std::string(ARG_EMBEDDING_SIZE) + ": " + std::to_string(embeddingSize) + ", ";
   rtn += std::string(ARG_LEARNING_RATE) + ": " + std::to_string(learningRate) + ", ";
   rtn += std::string(ARG_MARGIN) + ": " + std::to_string(margin) + ", ";
   rtn += std::string(ARG_METHOD) + ": " + std::to_string(method) + ", ";
   rtn += std::string(ARG_NUM_BATCHES) + ": " + std::to_string(numBatches) + ", ";
   rtn += std::string(ARG_MAX_EPOCHS) + ": " + std::to_string(maxEpochs) + ", ";
   rtn += std::string(ARG_DISTANCE_TYPE) + ": " + std::to_string(distanceType) + "]";

   return rtn;
}

TrainerArguments parseArgs(int argc, char** argv) {
   int index = argpos((char*)ARG_HELP, argc, argv);
   if (index != -1) {
      printUsage((char*)argv[0]);
      exit(0);
   }

   TrainerArguments parsedArgs;
   index = argpos((char*)ARG_DATA_DIR, argc, argv);
   if (index != -1) {
      parsedArgs.dataDir = argv[index + 1];
   }

   index = argpos((char*)ARG_OUT_DIR, argc, argv);
   if (index != -1) {
      parsedArgs.outputDir = argv[index + 1];
   }

   index = argpos((char*)ARG_EMBEDDING_SIZE, argc, argv);
   if (index != -1) {
      parsedArgs.embeddingSize = atoi(argv[index + 1]);
   }

   index = argpos((char*)ARG_LEARNING_RATE, argc, argv);
   if (index != -1) {
      parsedArgs.learningRate = atof(argv[index + 1]);
   }

   index = argpos((char*)ARG_MARGIN, argc, argv);
   if (index != -1) {
      parsedArgs.margin = atof(argv[index + 1]);
   }

   index = argpos((char*)ARG_METHOD, argc, argv);
   if (index != -1) {
      parsedArgs.method = atoi(argv[index + 1]);
   }

   index = argpos((char*)ARG_NUM_BATCHES, argc, argv);
   if (index != -1) {
      parsedArgs.numBatches = atoi(argv[index + 1]);
   }

   index = argpos((char*)ARG_MAX_EPOCHS, argc, argv);
   if (index != -1) {
      parsedArgs.maxEpochs = atoi(argv[index + 1]);
   }

   index = argpos((char*)ARG_DISTANCE_TYPE, argc, argv);
   if (index != -1) {
      parsedArgs.distanceType = atoi(argv[index + 1]);
   }

   return parsedArgs;
}

// Param is argv[0].
void printUsage(char* invokedFile) {
   printf("USAGE: %s [option value] ...\n", invokedFile);
   printf("       %s --help\n", invokedFile);
   printf("All options require a value.\n");
   printf("Options:\n");
   printf("   --%s\n", ARG_DATA_DIR);
   printf("   --%s\n", ARG_OUT_DIR);
   printf("   --%s\n", ARG_EMBEDDING_SIZE);
   printf("   --%s\n", ARG_LEARNING_RATE);
   printf("   --%s\n", ARG_MARGIN);
   printf("   --%s\n", ARG_METHOD);
   printf("   --%s\n", ARG_NUM_BATCHES);
   printf("   --%s\n", ARG_MAX_EPOCHS);
   printf("   --%s\n", ARG_DISTANCE_TYPE);
}

}   // namespace common
