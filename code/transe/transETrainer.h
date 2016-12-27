#ifndef TRANSE_TRANSETRAINER_H_
#define TRANSE_TRANSETRAINER_H_

#include <iostream>
#include <cstring>
#include <cstdio>
#include <map>
#include <vector>
#include <string>
#include <ctime>
#include <cmath>
#include <cstdlib>

#include "common/trainer.h"
#include "common/utils.h"

namespace transe {

class TransETrainer : public common::Trainer {
    public:
        explicit TransETrainer(int embeddingSize,  double learningRate, double margin, int method)
                : common::Trainer(embeddingSize, learningRate, margin, method) {}

    protected:
        // The next values for the embeddings.
        std::vector<std::vector<double>> relation_vec_next_;
        std::vector<std::vector<double>> entity_vec_next_;

        double initialEmbeddingValue() override;
        void gradientUpdate(int head, int tail, int relation, bool corrupted) override;
        void postbatch() override;
        void prebatch() override;
        double tripleEnergy(int head, int tail, int relation) override;
};

}  // namespace transe

#endif  // TRANSE_TRANSETRAINER_H_
