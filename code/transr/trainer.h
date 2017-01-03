#ifndef TRANSR_TRAINER_H_
#define TRANSR_TRAINER_H_

#include <vector>
#include <string>

#include "common/args.h"
#include "common/trainer.h"

namespace transr {

class Trainer : public common::Trainer {
   public:
      explicit Trainer(common::EmbeddingArguments args);

      void write() override;

   protected:
      int distanceType_;

      // Info on the data to use when seeding the emtity/relation embeddings.
      std::string seedDataDir_;
      int seedMethod_;

      std::vector<std::vector<std::vector<double>>> weights_;

      // The next values for the embeddings.
      std::vector<std::vector<double>> relationVec_next_;
      std::vector<std::vector<double>> entityVec_next_;
      std::vector<std::vector<std::vector<double>>> weights_next_;

      void transRNorm(std::vector<double> &a, std::vector<std::vector<double> > &b);

      void gradientUpdate(int head, int tail, int relation, bool corrupted) override;
      double initialEmbeddingValue() override;
      void postbatch() override;
      void prebatch() override;
      void prepTrain() override;
      double tripleEnergy(int head, int tail, int relation) override;
};

}  // namespace transr

#endif  // TRANSR_TRAINER_H_
