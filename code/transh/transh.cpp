#include "transh/transh.h"

#include <vector>
#include <cmath>

#include "common/utils.h"

namespace transh {

double tripleEnergy(int head, int tail, int relation,
                    int embeddingSize,
                    std::vector<std::vector<double>>& entityVec,
                    std::vector<std::vector<double>>& relationVec,
                    std::vector<std::vector<double>>& weights) {
    double headSum = 0;
    double tailSum = 0;

    for (int i = 0; i < embeddingSize; i++) {
        headSum += weights[relation][i] * entityVec[head][i];
        tailSum += weights[relation][i] * entityVec[tail][i];
    }

    double energy = 0;
    for (int i = 0; i < embeddingSize; i++) {
        energy += std::fabs(entityVec[tail][i] - tailSum * weights[relation][i] - (entityVec[head][i] - headSum * weights[relation][i]) - relationVec[relation][i]);
    }

    return energy;
}

}  // namespace transh
