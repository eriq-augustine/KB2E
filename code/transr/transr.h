#ifndef TRANSH_TRANSR_H_
#define TRANSH_TRANSR_H_

#include <vector>

namespace transr {

// TODO(eriq): Go to own file.
double tripleEnergy(int head, int tail, int relation,
                    int embeddingSize,
                    std::vector<std::vector<double>>& entityVec,
                    std::vector<std::vector<double>>& relationVec,
                    std::vector<std::vector<std::vector<double>>>& weights,
                    int distanceType,
                    std::vector<double>& headVec, std::vector<double>& tailVec);

}  // namespace transr

#endif  // TRANSH_TRANSR_H_
