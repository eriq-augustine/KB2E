#include "transe/transETrainer.h"

#include <map>
#include <vector>
#include <string>
#include <cmath>

#include "common/constants.h"
#include "common/trainer.h"
#include "common/utils.h"
#include "transe/transe.h"

namespace transe {

TransETrainer::TransETrainer(common::TrainerArguments args)
      : common::Trainer(args) {
   distanceType_ = args.distanceType;
}

double TransETrainer::initialEmbeddingValue() {
   return common::randn(0, 1.0 / embeddingSize_, -6 / std::sqrt(embeddingSize_), 6 / std::sqrt(embeddingSize_));
}

void TransETrainer::gradientUpdate(int head, int tail, int relation, bool corrupted) {
   double modifier = corrupted ? 1.0 : -1.0;

   for (int i = 0; i < embeddingSize_; i++) {
         double x = 2.0 * (entity_vec_[tail][i] - entity_vec_[head][i] - relation_vec_[relation][i]);
         if (distanceType_ == L1_DISTANCE) {
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
   return transe::tripleEnergy(head, tail, relation, embeddingSize_, entity_vec_, relation_vec_, (distanceType_ == L1_DISTANCE));
}

}  // namespace transe
