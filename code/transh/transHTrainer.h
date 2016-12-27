#ifndef TRANSH_TRANSHTRAINER_H_
#define TRANSH_TRANSHTRAINER_H_

#include <vector>

#include "common/trainer.h"
#include "common/utils.h"

namespace transh {

class TransHTrainer : public common::Trainer {
   public:
      explicit TransHTrainer(common::TrainerArguments args) : common::Trainer(args) {}
      void write() override;

   protected:
      std::vector<std::vector<double>> weights_;

      // The next values for the embeddings.
      std::vector<std::vector<double>> relation_vec_next_;
      std::vector<std::vector<double>> entity_vec_next_;
      std::vector<std::vector<double>> weights_next_;

      void gradientUpdate(int head, int tail, int relation, bool corrupted) override;
      double initialEmbeddingValue() override;
      void postbatch() override;
      void prebatch() override;
      void prepTrain() override;
      double tripleEnergy(int head, int tail, int relation) override;
};

}  // namespace transh

#endif  // TRANSH_TRANSHTRAINER_H_
