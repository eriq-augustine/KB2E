#ifndef TRANSH_EVALUATION_H_
#define TRANSH_EVALUATION_H_

#include <map>
#include <vector>

#include "common/evaluation.h"

namespace transh {

class Evaluation : public common::EmbeddingEvaluation {
   public:
      explicit Evaluation(common::EmbeddingArguments args);

   private:
      std::string weightEmbeddingPath_;

      std::vector<std::vector<double>> weights_;

      double tripleEnergy(int head, int tail, int relation) override;
      void loadEmbeddings() override;
};

}  // namespace transh

#endif  // TRANSH_EVALUATION_H_
