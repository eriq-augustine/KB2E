#include "transr/transr.h"

#include <cmath>
#include <vector>
#include <string>

#include "common/constants.h"
#include "common/trainer.h"
#include "common/utils.h"

namespace transr {

double tripleEnergy(int head, int tail, int relation,
                    int embeddingSize,
                    std::vector<std::vector<double>>& entityVec,
                    std::vector<std::vector<double>>& relationVec,
                    std::vector<std::vector<std::vector<double>>>& weights,
                    int distanceType) {
   std::vector<double> headVec;
   std::vector<double> tailVec;

   headVec.resize(embeddingSize);
   tailVec.resize(embeddingSize);

   for (int i = 0; i < embeddingSize; i++) {
      for (int j = 0; j < embeddingSize; j++) {
         headVec[i] += weights[relation][j][i] * entityVec[head][j];
         tailVec[i] += weights[relation][j][i] * entityVec[tail][j];
      }
   }

   double sum = 0;
   if (distanceType == L1_DISTANCE) {
      for (int i = 0; i < embeddingSize; i++) {
         sum += fabs(tailVec[i] - headVec[i] - relationVec[relation][i]);
      }
   } else {
      for (int i=0; i<embeddingSize; i++) {
         sum += common::sqr(tailVec[i] - headVec[i] - relationVec[relation][i]);
      }
   }

   return sum;
}

}  // namespace transr
