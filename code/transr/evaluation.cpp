#include "transr/evaluation.h"

#include <cstdio>
#include <vector>
#include <string>

#include "common/args.h"
#include "common/constants.h"
#include "common/evaluation.h"
#include "common/utils.h"
#include "transr/transr.h"

namespace transr {

Evaluation::Evaluation(common::EmbeddingArguments args)
      : common::EmbeddingEvaluation(args) {
   distanceType_ = args.distanceType;
   seedDataDir_ = args.seedDataDir;
   seedMethod_ = args.seedMethod;

   // Initialize some vectors that will be reused.
   headWorkVec_.resize(embeddingSize_);
   tailWorkVec_.resize(embeddingSize_);
}

double Evaluation::tripleEnergy(int head, int tail, int relation) {
   return transr::tripleEnergy(head, tail, relation,
                               embeddingSize_,
                               entityVec_, relationVec_, weights_,
                               distanceType_,
                               headWorkVec_, tailWorkVec_);
}

void Evaluation::loadEmbeddings() {
   std::string weightEmbeddingPath = outputDir_ + "/" + WEIGHT_EMBEDDING_FILE_BASENAME + "." + METHOD_TO_STRING(method_);
   if (!common::fileExists(weightEmbeddingPath)) {
      printf("Could not find weight embedding file: %s. Make sure to specify the path and/or train.\n", weightEmbeddingPath.c_str());
      exit(2);
   }

   common::EmbeddingEvaluation::loadEmbeddings();

   FILE* weightEmbeddingFile = fopen(weightEmbeddingPath.c_str(), "r");
   weights_.resize(numRelations_);
   for (int i = 0; i < numRelations_; i++) {
      weights_[i].resize(embeddingSize_);

      for (int j = 0; j < embeddingSize_; j++) {
         weights_[i][j].resize(embeddingSize_);

         for (int k = 0; k < embeddingSize_; k++) {
            fscanf(weightEmbeddingFile, "%lf", &weights_[i][j][k]);
         }
      }
   }
   fclose(weightEmbeddingFile);
}

}  // namespace transr
