#ifndef TRANSE_EVALUATION_H_
#define TRANSE_EVALUATION_H_

#include <map>
#include <vector>

#include "common/args.h"
#include "common/evaluation.h"

namespace transe {

class Evaluation : public common::EmbeddingEvaluation {
   public:
      explicit Evaluation(common::EmbeddingArguments args);

   private:
      int distanceType_;

      double tripleEnergy(int head, int tail, int relation) override;
};

}  // namespace transe

#endif  // TRANSE_EVALUATION_H_
