#include "transe/transe.h"

#include <map>
#include <vector>
#include <cmath>

#include "common/utils.h"

namespace transe {

double tripleEnergy(int head, int tail, int relation,
                    int embeddingSize,
                    std::vector<std::vector<double>>& entityVec,
                    std::vector<std::vector<double>>& relationVec,
                    bool useL1) {
   double energy = 0;

   if (useL1) {
      for (int i = 0; i < embeddingSize; i++) {
         energy += std::fabs(entityVec[tail][i] - entityVec[head][i] - relationVec[relation][i]);
      }
   } else {
      for (int i = 0; i < embeddingSize; i++) {
         energy += common::sqr(entityVec[tail][i] - entityVec[head][i] - relationVec[relation][i]);
      }
   }

   return energy;
}

}  // namespace transe
