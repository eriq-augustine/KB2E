#include "common/trainer.h"

#include <iostream>
#include <cstring>
#include <map>
#include <vector>
#include <string>

#include "common/constants.h"
#include "common/utils.h"

namespace common {

Trainer::Trainer(TrainerArguments args) {
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
   relation_vec_.resize(numRelations_);
   for (int i = 0; i <relation_vec_.size(); i++) {
      relation_vec_[i].resize(embeddingSize_);
   }

   entity_vec_.resize(numEntities_);
   for (int i = 0; i <entity_vec_.size(); i++) {
      entity_vec_[i].resize(embeddingSize_);
   }

   for (int i = 0; i < numRelations_; i++) {
      for (int j = 0; j < embeddingSize_; j++) {
         relation_vec_[i][j] = initialEmbeddingValue();
      }
      norm(relation_vec_[i]);
   }

   for (int i = 0; i < numEntities_; i++) {
      for (int j = 0; j < embeddingSize_; j++) {
         entity_vec_[i][j] = initialEmbeddingValue();
      }
      norm(entity_vec_[i]);
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
   FILE* relationOutFile = fopen((outputDir_ + "/" + RELATION_OUT_FILE_BASENAME + "." + methodName()).c_str(), "w");
   for (int i = 0; i < numRelations_; i++) {
      for (int j = 0; j < embeddingSize_; j++) {
         fprintf(relationOutFile, "%.6lf\t", relation_vec_[i][j]);
      }
      fprintf(relationOutFile, "\n");
   }
   fclose(relationOutFile);

   FILE* entityOutFile = fopen((outputDir_ + "/" + ENTITY_OUT_FILE_BASENAME + "." + methodName()).c_str(), "w");
   for (int i = 0; i < numEntities_; i++) {
      for (int j = 0; j < embeddingSize_; j++) {
         fprintf(entityOutFile, "%.6lf\t", entity_vec_[i][j]);
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

   int entityId;
   char headIdStringBuf[ID_STRING_MAX_LEN];

   // TODO(eriq): Better error handling with files.

   FILE* entityIdFile = fopen((dataDir_ + "/" + ENTITY_ID_FILE).c_str(), "r");
   while (fscanf(entityIdFile, "%s\t%d", headIdStringBuf, &entityId) == 2) {
      std::string entityIdString = headIdStringBuf;
      entity2id[entityIdString] = entityId;
   }
   fclose(entityIdFile);

   FILE* relationIdFile = fopen((dataDir_ + "/" + RELATION_ID_FILE).c_str(), "r");
   while (fscanf(relationIdFile, "%s\t%d", headIdStringBuf, &entityId) == 2) {
      std::string entityIdString = headIdStringBuf;
      relation2id[entityIdString] = entityId;
   }
   fclose(relationIdFile);

   char tailIdStringBuf[ID_STRING_MAX_LEN];
   char relationIdStringBuf[ID_STRING_MAX_LEN];

   FILE* trainFile = fopen((dataDir_ + "/" + TRAIN_FILE).c_str(), "r");
   while (fscanf(trainFile,"%s\t%s\t%s", headIdStringBuf, tailIdStringBuf, relationIdStringBuf) == 3) {
      std::string headIdString = headIdStringBuf;
      std::string tailIdString = tailIdStringBuf;
      std::string relationIdString = relationIdStringBuf;

      bool fail = false;
      if (entity2id.count(headIdString) == 0) {
         std::cout << "Head entity found in training set that was not found in the identity file: " << headIdString << std::endl;
         fail = true;
      }

      if (entity2id.count(tailIdString) == 0) {
         std::cout << "Tail entity found in training set that was not found in the identity file: " << tailIdString << std::endl;
         fail = true;
      }

      if (relation2id.count(relationIdString) == 0) {
         std::cout << "Relation found in training set that was not found in the identity file: " << relationIdString << std::endl;
         fail = true;
      }

      if (fail) {
         continue;
      }

      headCooccurrence[relation2id[relationIdString]][entity2id[headIdString]]++;
      tailCooccurrence[relation2id[relationIdString]][entity2id[tailIdString]]++;

      add(entity2id[headIdString], entity2id[tailIdString], relation2id[relationIdString]);
   }
   fclose(trainFile);

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
