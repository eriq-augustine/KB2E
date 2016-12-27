#ifndef COMMON_UTILS_H_
#define COMMON_UTILS_H_

#include <vector>

#define PI 3.1415926535897932384626433832795

namespace common {

// normal distribution
double rand(double min, double max);
double normal(double x, double miu, double sigma);
double randn(double miu,double sigma, double min, double max);
double sqr(double x);
double vec_len(std::vector<double> &a);
int argpos(char *str, int argc, char **argv);
void norm(std::vector<double> &a);
int randMax(int x);

}   // namespace common

#endif  // COMMON_UTILS_H_
