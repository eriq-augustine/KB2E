#include "common/utils.h"

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>

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

int argpos(char *str, int argc, char **argv) {
   int i;

   for (i = 1; i < argc; i++) {
      if (!std::strcmp(str, argv[i])) {
         if (i == argc - 1) {
            printf("Argument missing for %s\n", str);
            exit(1);
         }

         return i;
      }
   }

   return -1;
}

void norm(std::vector<double> &a) {
   double x = vec_len(a);
   if (x > 1) {
      for (int ii = 0; ii < a.size(); ii++) {
         a[ii] /= x;
      }
   }
}

int randMax(int x) {
   int res = (std::rand() * std::rand()) % x;
   while (res < 0) {
      res += x;
   }

   return res;
}

}   // namespace common
