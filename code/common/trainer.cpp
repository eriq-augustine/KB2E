#include "common/trainer.h"

#include <iostream>
#include <cstring>
#include <map>
#include <vector>
#include <string>

#include "common/constants.h"
#include "common/loader.h"
#include "common/utils.h"

namespace common {

Trainer::Trainer(EmbeddingArguments args) {
   dataDir_ = args.dataDir;
   outputDir_ = args.outputDir;
   embeddingSize_ = args.embeddingSize;
   learningRate_ = args.learningRate;
   margin_ = args.margin;
   method_ = args.method;
   numBatches_ = args.numBatches;
   maxEpochs_ = args.maxEpochs;
}

void Trainer::add(int head, int tail, int relation) {
   heads_.push_back(head);
   tails_.push_back(tail);
   relations_.push_back(relation);

   triples_[std::make_pair(head, relation)][tail] = 1;
}

void Trainer::prepTrain() {
   relationVec_.resize(numRelations_);
   for (int i = 0; i <relationVec_.size(); i++) {
      relationVec_[i].resize(embeddingSize_);
   }

   entityVec_.resize(numEntities_);
   for (int i = 0; i <entityVec_.size(); i++) {
      entityVec_[i].resize(embeddingSize_);
   }

   for (int i = 0; i < numRelations_; i++) {
      for (int j = 0; j < embeddingSize_; j++) {
         relationVec_[i][j] = initialEmbeddingValue();
      }
      norm(relationVec_[i]);
   }

   for (int i = 0; i < numEntities_; i++) {
      for (int j = 0; j < embeddingSize_; j++) {
         entityVec_[i][j] = initialEmbeddingValue();
      }
      norm(entityVec_[i]);
   }
}

void Trainer::train() {
   prepTrain();
   bfgs();
}

std::string Trainer::methodName() {
   if (method_ == METHOD_BERN) {
      return METHOD_NAME_BERN;
   }

   return METHOD_NAME_UNIF;
}

void Trainer::bfgs() {
   int batchsize = heads_.size() / numBatches_;

   for (int epoch = 0; epoch < maxEpochs_; epoch++) {
      double loss = 0;

      for (int batch = 0; batch < numBatches_; batch++) {
         prebatch();

         for (int k = 0; k < batchsize; k++)
         {
            int i = randMax(heads_.size());
            int j = randMax(numEntities_);

            double pr = 1000 * relationTailMeanCooccurrence_[relations_[i]] / (relationTailMeanCooccurrence_[relations_[i]] + relationHeadMeanCooccurrence_[relations_[i]]);

            if (method_ == METHOD_UNIF) {
               pr = 500;
            }

            if (std::rand() % 1000 < pr) {
               while (triples_[std::make_pair(heads_[i], relations_[i])].count(j) > 0) {
                  j = randMax(numEntities_);
               }
               loss += train_kb(heads_[i], tails_[i], relations_[i], heads_[i], j, relations_[i]);
            } else {
               while (triples_[std::make_pair(j, relations_[i])].count(tails_[i]) > 0) {
                  j = randMax(numEntities_);
               }
               loss += train_kb(heads_[i], tails_[i], relations_[i], j, tails_[i], relations_[i]);
            }
         }

         postbatch();
      }

      // TODO(eriq): Debug
      std::cout << "epoch: " << epoch << ", loss: " << loss << std::endl;
   }
}

void Trainer::write() {
   FILE* relationOutFile = fopen((outputDir_ + "/" + RELATION_EMBEDDING_FILE_BASENAME + "." + methodName()).c_str(), "w");
   for (int i = 0; i < numRelations_; i++) {
      for (int j = 0; j < embeddingSize_; j++) {
         fprintf(relationOutFile, "%.6lf\t", relationVec_[i][j]);
      }
      fprintf(relationOutFile, "\n");
   }
   fclose(relationOutFile);

   FILE* entityOutFile = fopen((outputDir_ + "/" + ENTITY_EMBEDDING_FILE_BASENAME + "." + methodName()).c_str(), "w");
   for (int i = 0; i < numEntities_; i++) {
      for (int j = 0; j < embeddingSize_; j++) {
         fprintf(entityOutFile, "%.6lf\t", entityVec_[i][j]);
      }
      fprintf(entityOutFile, "\n");
   }
   fclose(entityOutFile);
}

// a is normal, b is corrupted.
double Trainer::train_kb(int aHead, int aTail, int aRelation, int bHead, int bTail, int bRelation) {
   double loss = 0;
   double normalEnergy = tripleEnergy(aHead, aTail, aRelation);
   double corruptedEnergy = tripleEnergy(bHead, bTail, bRelation);

   // printf("   Normal:    (%d, %d, %d): %f\n", aHead, aTail, aRelation, normalEnergy);
   // printf("   Corrupted: (%d, %d, %d): %f\n", bHead, bTail, bRelation, corruptedEnergy);

   if (normalEnergy + margin_ > corruptedEnergy) {
      loss = margin_ + normalEnergy - corruptedEnergy;
      gradientUpdate(aHead, aTail, aRelation, false);
      gradientUpdate(bHead, bTail, bRelation, true);
   }

   return loss;
}

void Trainer::loadFiles() {
   std::map<std::string, int> entity2id;
   std::map<std::string, int> relation2id;

   // relation/entity co-orrirance counts.
   // <relation, <entity, count>>
   std::map<int, std::map<int, int>> headCooccurrence;
   std::map<int, std::map<int, int>> tailCooccurrence;

   loadIdFile(dataDir_ + "/" + ENTITY_ID_FILE, entity2id);
   loadIdFile(dataDir_ + "/" + RELATION_ID_FILE, relation2id);

   std::function<void(int, int, int)> tripleCallback = [=, &headCooccurrence, &tailCooccurrence](int head, int tail, int relation) {
      headCooccurrence[relation][head]++;
      tailCooccurrence[relation][tail]++;
      this->add(head, tail, relation);
   };

   loadTripleFile(dataDir_ + "/" + TRAIN_FILE, entity2id, relation2id, tripleCallback);

   for (int i = 0; i < relation2id.size(); i++) {
      // Sum the number of times this relation (i) appears.
      double totalHeadRelationCooccurrences = 0;
      for (std::map<int, int>::iterator it = headCooccurrence[i].begin(); it != headCooccurrence[i].end(); it++) {
         totalHeadRelationCooccurrences += it->second;
      }

      if (headCooccurrence[i].size() == 0) {
         relationHeadMeanCooccurrence_[i] = 0;
      } else {
         relationHeadMeanCooccurrence_[i] = totalHeadRelationCooccurrences / headCooccurrence[i].size();
      }

      double totalTailRelationCooccurrences = 0;
      for (std::map<int, int>::iterator it = tailCooccurrence[i].begin(); it != tailCooccurrence[i].end(); it++) {
         totalTailRelationCooccurrences += it->second;
      }

      if (tailCooccurrence[i].size() == 0) {
         relationTailMeanCooccurrence_[i] = 0;
      } else {
         relationTailMeanCooccurrence_[i] = totalTailRelationCooccurrences / tailCooccurrence[i].size();
      }
   }

   numRelations_ = relation2id.size();
   numEntities_ = entity2id.size();

   std::cout << "Number of Relations: " << relation2id.size() << std::endl;
   std::cout << "Number of Entities: " << entity2id.size() << std::endl;
}

}  // namespace common
