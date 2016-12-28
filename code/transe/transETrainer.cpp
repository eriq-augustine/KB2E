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
         double x = 2.0 * (entityVec_[tail][i] - entityVec_[head][i] - relationVec_[relation][i]);
         if (distanceType_ == L1_DISTANCE) {
            if (x > 0) {
               x = 1;
            } else {
               x = -1;
            }
         }

         relationVec_next_[relation][i] -= modifier * learningRate_ * x;
         entityVec_next_[head][i] -= modifier * learningRate_ * x;
         entityVec_next_[tail][i] += modifier * learningRate_ * x;
   }

   common::norm(relationVec_next_[relation]);
   common::norm(entityVec_next_[head]);
   common::norm(entityVec_next_[tail]);
}

void TransETrainer::postbatch() {
   relationVec_ = relationVec_next_;
   entityVec_ = entityVec_next_;
}

void TransETrainer::prebatch() {
   relationVec_next_ = relationVec_;
   entityVec_next_ = entityVec_;
}

double TransETrainer::tripleEnergy(int head, int tail, int relation) {
   return transe::tripleEnergy(head, tail, relation, embeddingSize_, entityVec_, relationVec_, (distanceType_ == L1_DISTANCE));
}

}  // namespace transe
