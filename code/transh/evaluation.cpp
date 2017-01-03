#include "transh/evaluation.h"

#include "common/args.h"
#include "common/constants.h"
#include "common/evaluation.h"
#include "common/utils.h"
#include "transh/transh.h"

namespace transh {

Evaluation::Evaluation(common::EmbeddingArguments args)
      : common::EmbeddingEvaluation(args) {
   weightEmbeddingPath_ = outputDir_ + "/" + WEIGHT_EMBEDDING_FILE_BASENAME + "." + METHOD_TO_STRING(method_);
}

double Evaluation::tripleEnergy(int head, int tail, int relation) {
   return transh::tripleEnergy(head, tail, relation, embeddingSize_, entityVec_, relationVec_, weights_);
}

void Evaluation::loadEmbeddings() {
   if (!common::fileExists(weightEmbeddingPath_)) {
      printf("Could not find weight embedding file: %s. Make sure to specify the path and/or train.\n", weightEmbeddingPath_.c_str());
      exit(2);
   }

   common::EmbeddingEvaluation::loadEmbeddings();

   FILE* weightEmbeddingFile = fopen(weightEmbeddingPath_.c_str(), "r");
   weights_.resize(numRelations_);
   for (int i = 0; i < numRelations_; i++) {
      weights_[i].resize(embeddingSize_);
      for (int j = 0; j < embeddingSize_; j++) {
         fscanf(weightEmbeddingFile, "%lf", &weights_[i][j]);
      }
   }
   fclose(weightEmbeddingFile);
}

}  // namespace transh
