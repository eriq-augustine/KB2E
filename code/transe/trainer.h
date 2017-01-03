#ifndef TRANSE_TRAINER_H_
#define TRANSE_TRAINER_H_

#include <vector>

#include "common/args.h"
#include "common/trainer.h"
#include "common/utils.h"

namespace transe {

class Trainer : public common::Trainer {
   public:
      explicit Trainer(common::EmbeddingArguments args);

   protected:
      // The next values for the embeddings.
      std::vector<std::vector<double>> relationVec_next_;
      std::vector<std::vector<double>> entityVec_next_;

      int distanceType_;

      double initialEmbeddingValue() override;
      void gradientUpdate(int head, int tail, int relation, bool corrupted) override;
      void postbatch() override;
      void prebatch() override;
      double tripleEnergy(int head, int tail, int relation) override;
};

}  // namespace transe

#endif  // TRANSE_TRAINER_H_
