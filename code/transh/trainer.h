#ifndef TRANSH_TRAINER_H_
#define TRANSH_TRAINER_H_

#include <vector>

#include "common/trainer.h"
#include "common/utils.h"

namespace transh {

class Trainer : public common::Trainer {
   public:
      explicit Trainer(common::EmbeddingArguments args) : common::Trainer(args) {}
      void write() override;

   protected:
      std::vector<std::vector<double>> weights_;

      // The next values for the embeddings.
      std::vector<std::vector<double>> relationVec_next_;
      std::vector<std::vector<double>> entityVec_next_;
      std::vector<std::vector<double>> weights_next_;

      void gradientUpdate(int head, int tail, int relation, bool corrupted) override;
      double initialEmbeddingValue() override;
      void postbatch() override;
      void prebatch() override;
      void prepTrain() override;
      double tripleEnergy(int head, int tail, int relation) override;
};

}  // namespace transh

#endif  // TRANSH_TRAINER_H_
