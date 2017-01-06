#include "transr/trainer.h"

#include <cmath>
#include <iostream>
#include <vector>
#include <string>

#include "common/args.h"
#include "common/constants.h"
#include "common/utils.h"
#include "transr/transr.h"

namespace transr {

Trainer::Trainer(common::EmbeddingArguments args)
      : common::Trainer(args) {
   distanceType_ = args.distanceType;
   seedDataDir_ = args.seedDataDir;
   seedMethod_ = args.seedMethod;

   // Initialize some vectors that will be reused.
   headWorkVec_.resize(embeddingSize_);
   tailWorkVec_.resize(embeddingSize_);
}

double Trainer::tripleEnergy(int head, int tail, int relation) {
   return transr::tripleEnergy(head, tail, relation,
                               embeddingSize_,
                               entityVec_, relationVec_, weights_,
                               distanceType_,
                               headWorkVec_, tailWorkVec_);
}

// TODO(eriq): Examine, is this some special norm?
void Trainer::transRNorm(std::vector<double> &a, std::vector<std::vector<double> > &b) {
   while (true) {
      double x = 0;
      for (int i = 0; i < embeddingSize_; i++) {
         double tmp = 0;
         for (int j = 0; j < embeddingSize_; j++) {
            tmp += b[j][i] * a[j];
         }
         x += common::sqr(tmp);
      }

      if (x <= 1) {
         break;
      }

      double lambda = 1;
      for (int i = 0; i < embeddingSize_; i++) {
         double tmp = 0;
         for (int j = 0; j < embeddingSize_; j++) {
            tmp += b[j][i] * a[j];
         }

         tmp *= 2;
         for (int j=0; j<embeddingSize_; j++) {
            b[j][i] -= learningRate_ * lambda * tmp * a[j];
            a[j] -= learningRate_ * lambda * tmp * b[j][i];
         }
      }
   }
}

double Trainer::initialEmbeddingValue() {
   return common::randn(0, 1.0 / embeddingSize_, -1, 1);
}

void Trainer::prepTrain() {
   common::Trainer::prepTrain();

   weights_.resize(numRelations_);
   for (int i = 0; i < numRelations_; i++) {
      weights_[i].resize(embeddingSize_);
      for (int j = 0; j < embeddingSize_; j++) {
         weights_[i][j].resize(embeddingSize_);
         for (int k = 0; k < embeddingSize_; k++) {
            if (k == j) {
               weights_[i][j][k] = 1.0;
            } else {
               weights_[i][j][k] = 0.0;
            }
         }
      }
   }

   // Preload the entity and relation embeddings with previous data.
   // Historically, we use data from TransE using the unif (uniform) method.
   std::string path = seedDataDir_ + "/" + ENTITY_EMBEDDING_FILE_BASENAME + "." + METHOD_TO_STRING(seedMethod_);
   FILE* entitySeedFile = fopen(path.c_str(), "r");
   for (int i = 0; i < numEntities_; i++) {
      for (int j = 0; j < embeddingSize_; j++) {
         fscanf(entitySeedFile, "%lf", &entityVec_[i][j]);
      }
      common::norm(entityVec_[i], false);
   }
   fclose(entitySeedFile);

   path = seedDataDir_ + "/" + RELATION_EMBEDDING_FILE_BASENAME + "." + METHOD_TO_STRING(seedMethod_);
   FILE* relationSeedFile = fopen(path.c_str(), "r");
   for (int i = 0; i < numRelations_; i++) {
      for (int j = 0; j < embeddingSize_; j++) {
         fscanf(relationSeedFile, "%lf", &relationVec_[i][j]);
      }
   }
   fclose(relationSeedFile);
}

void Trainer::postbatch() {
   relationVec_ = relationVec_next_;
   entityVec_ = entityVec_next_;
   weights_ = weights_next_;
}

void Trainer::prebatch() {
   relationVec_next_ = relationVec_;
   entityVec_next_ = entityVec_;
   weights_next_ = weights_;
}

void Trainer::write() {
   common::Trainer::write();

   std::string path = outputDir_ + "/" + WEIGHT_EMBEDDING_FILE_BASENAME + "." + methodName();
   FILE* weightOutFile = fopen(path.c_str(), "w");
   for (int i = 0; i < numRelations_; i++) {
      for (int j = 0; j < embeddingSize_; j++) {
         for (int k = 0; k < embeddingSize_; k++) {
            fprintf(weightOutFile, "%.6lf\t", weights_[i][j][k]);
         }
         fprintf(weightOutFile, "\n");
      }
   }
   fclose(weightOutFile);
}

void Trainer::gradientUpdate(int head, int tail, int relation, bool corrupted) {
   double beta = corrupted ? 1.0 : -1.0;

   for (int i = 0; i < embeddingSize_; i++) {
      double headSum = 0;
      double tailSum = 0;

      for (int j = 0; j < embeddingSize_; j++) {
         headSum += weights_[relation][j][i] * entityVec_[head][j];
         tailSum += weights_[relation][j][i] * entityVec_[tail][j];
      }

      double x = 2.0 * (tailSum - headSum - relationVec_[relation][i]);

      if (distanceType_ == L1_DISTANCE) {
         if (x > 0) {
            x = 1;
         } else {
            x = -1;
         }
      }

      for (int j = 0; j < embeddingSize_; j++) {
         weights_next_[relation][j][i] -= beta * learningRate_ * x * (entityVec_[head][j] - entityVec_[tail][j]);
         entityVec_next_[head][j] -= beta * learningRate_ * x * weights_[relation][j][i];
         entityVec_next_[tail][j] += beta * learningRate_ * x * weights_[relation][j][i];
      }
      relationVec_next_[relation][i] -= beta * learningRate_ * x;
   }

   common::norm(relationVec_next_[relation], false);
   common::norm(entityVec_next_[head], false);
   common::norm(entityVec_next_[tail], false);

   for (int i = 0; i < embeddingSize_; i++) {
      common::norm(weights_next_[relation][i], false);
   }

   // TODO(eriq): There was a lot of strange normalization in the old version.
   //  We may need to double check all of this.

   transRNorm(entityVec_next_[head], weights_next_[relation]);
   transRNorm(entityVec_next_[tail], weights_next_[relation]);
   transRNorm(entityVec_next_[relation], weights_next_[relation]);
}

}  // namespace transr
