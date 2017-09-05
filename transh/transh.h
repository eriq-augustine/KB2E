#ifndef TRANSH_TRANSH_H_
#define TRANSH_TRANSH_H_

#include <vector>

namespace transh {

double tripleEnergy(int head, int tail, int relation,
                    int embeddingSize,
                    std::vector<std::vector<double>>& entityVec,
                    std::vector<std::vector<double>>& relationVec,
                    std::vector<std::vector<double>>& weights);

}  // namespace transh

#endif  // TRANSH_TRANSH_H_
