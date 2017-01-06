#ifndef TRANSR_EVALUATION_H_
#define TRANSR_EVALUATION_H_

#include <map>
#include <vector>

#include "common/args.h"
#include "common/evaluation.h"

namespace transr {

class Evaluation : public common::EmbeddingEvaluation {
   public:
      explicit Evaluation(common::EmbeddingArguments args);

   private:
      int distanceType_;

      // Info on the data to use when seeding the emtity/relation embeddings.
      std::string seedDataDir_;
      int seedMethod_;

      // Vectors that will be resued for energy calculation.
      // We will allocate once to speed up energy calculation.
      // Both will be |embeddingSize_| large.
      std::vector<double> headWorkVec_;
      std::vector<double> tailWorkVec_;

      std::vector<std::vector<std::vector<double>>> weights_;

      double tripleEnergy(int head, int tail, int relation) override;
      void loadEmbeddings() override;
};

}  // namespace transr

#endif  // TRANSR_EVALUATION_H_
