#include "transe/evaluation.h"

#include "common/constants.h"
#include "common/evaluation.h"
#include "transe/transe.h"

namespace transe {

Evaluation::Evaluation(common::EmbeddingArguments args)
      : common::EmbeddingEvaluation(args) {
   distanceType_ = args.distanceType;
}

double Evaluation::tripleEnergy(int head, int tail, int relation) {
   return transe::tripleEnergy(head, tail, relation, embeddingSize_, entityVec_, relationVec_, (distanceType_ == L1_DISTANCE));
}

}  // namespace transe
