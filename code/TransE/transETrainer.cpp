#include "TransE/transETrainer.h"

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

// TODO(eriq): Config
#define L1_FLAG true

namespace transe {

double TransETrainer::initialEmbeddingValue() {
    return common::randn(0, 1.0 / embeddingSize_, -6 / std::sqrt(embeddingSize_), 6 / std::sqrt(embeddingSize_));
}

void TransETrainer::gradientUpdate(int head, int tail, int relation, bool corrupted) {
   double modifier = corrupted ? 1 : -1;

   for (int i = 0; i < embeddingSize_; i++) {
         double x = 2 * (entity_vec_[tail][i] - entity_vec_[head][i] - relation_vec_[relation][i]);
         if (L1_FLAG) {
            if (x > 0) {
               x = 1;
            } else {
               x = -1;
            }
         }

         relation_vec_next_[relation][i] -= modifier * learningRate_ * x;
         entity_vec_next_[head][i] -= modifier * learningRate_ * x;
         entity_vec_next_[tail][i] += modifier * learningRate_ * x;
   }

   common::norm(relation_vec_next_[relation]);
   common::norm(entity_vec_next_[head]);
   common::norm(entity_vec_next_[tail]);
}

void TransETrainer::postbatch() {
    relation_vec_ = relation_vec_next_;
    entity_vec_ = entity_vec_next_;
}

void TransETrainer::prebatch() {
    relation_vec_next_ = relation_vec_;
    entity_vec_next_ = entity_vec_;
}

double TransETrainer::tripleEnergy(int head, int tail, int relation) {
    double energy = 0;

    if (L1_FLAG) {
        for (int i = 0; i < embeddingSize_; i++) {
            energy += std::fabs(entity_vec_[tail][i] - entity_vec_[head][i] - relation_vec_[relation][i]);
        }
    } else {
        for (int i = 0; i < embeddingSize_; i++) {
            energy += common::sqr(entity_vec_[tail][i] - entity_vec_[head][i] - relation_vec_[relation][i]);
        }
    }

    return energy;
}

}  // namespace transe
