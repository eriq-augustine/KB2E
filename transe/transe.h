#ifndef TRANSE_TRANSE_H_
#define TRANSE_TRANSE_H_

#include <vector>

namespace transe {

double tripleEnergy(int head, int tail, int relation,
                    int embeddingSize,
                    std::vector<std::vector<double>>& entityVec,
                    std::vector<std::vector<double>>& relationVec,
                    bool useL1);

}  // namespace transe

#endif  // TRANSE_TRANSE_H_
