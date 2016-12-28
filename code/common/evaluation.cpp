#include "common/evaluation.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <vector>
#include <string>

#include "common/constants.h"
#include "common/loader.h"
#include "common/utils.h"

namespace common {

double cmp(std::pair<int, double> a, std::pair<int, double> b) {
   return a.second < b.second;
}

EmbeddingEvaluation::EmbeddingEvaluation(EmbeddingArguments args) {
   dataDir_ = args.dataDir;
   embeddingSize_ = args.embeddingSize;
   method_ = args.method;
   outputDir_ = args.outputDir;

   relationEmbeddingPath_ = outputDir_ + "/" + RELATION_EMBEDDING_FILE_BASENAME + "." + METHOD_TO_STRING(method_);
   entityEmbeddingPath_ = outputDir_ + "/" + ENTITY_EMBEDDING_FILE_BASENAME + "." + METHOD_TO_STRING(method_);
}

void EmbeddingEvaluation::loadTriples() {
   std::map<std::string, int> relation2id;
   std::map<std::string, int> entity2id;

   loadIdFile(dataDir_ + "/" + ENTITY_ID_FILE, entity2id);
   loadIdFile(dataDir_ + "/" + RELATION_ID_FILE, relation2id);

   numEntities_ = entity2id.size();
   numRelations_ = relation2id.size();

   std::function<void(int, int, int)> tripleCallbackTrue = [this](int head, int tail, int relation) {
      this->add(head, tail, relation, true);
   };

   std::function<void(int, int, int)> tripleCallbackFalse = [this](int head, int tail, int relation) {
      this->add(head, tail, relation, false);
   };

   loadTripleFile(dataDir_ + "/" + TEST_FILE, entity2id, relation2id, tripleCallbackTrue);
   loadTripleFile(dataDir_ + "/" + TRAIN_FILE, entity2id, relation2id, tripleCallbackFalse);
   loadTripleFile(dataDir_ + "/" + VALID_FILE, entity2id, relation2id, tripleCallbackFalse);
}

void EmbeddingEvaluation::add(int head, int tail, int relation, bool addToWorkingSet) {
   if (addToWorkingSet) {
      heads_.push_back(head);
      relations_.push_back(relation);
      tails_.push_back(tail);
   }

   triples_[std::make_pair(head, relation)][tail] = 1;
}

void EmbeddingEvaluation::loadEmbeddings() {
   FILE* relationEmbeddingFile = fopen(relationEmbeddingPath_.c_str(), "r");
   relationVec_.resize(numRelations_);
   for (int i = 0; i < numRelations_; i++) {
      relationVec_[i].resize(embeddingSize_);
      for (int j = 0; j < embeddingSize_; j++) {
         fscanf(relationEmbeddingFile, "%lf", &relationVec_[i][j]);
      }
   }
   fclose(relationEmbeddingFile);

   FILE* entityEmbeddingFile = fopen(entityEmbeddingPath_.c_str(), "r");
   entityVec_.resize(numEntities_);
   for (int i = 0; i < numEntities_; i++) {
      entityVec_[i].resize(embeddingSize_);
      for (int j = 0; j < embeddingSize_; j++) {
         fscanf(entityEmbeddingFile, "%lf", &entityVec_[i][j]);
      }

      // TODO(eriq): I don't know what this check it.
      if (vec_len(entityVec_[i]) - 1 > 1e-3) {
         std::cout << "wrong_entity" << i << ' ' << vec_len(entityVec_[i]) << std::endl;
      }
   }
   fclose(entityEmbeddingFile);
}

// If |corruptHead| is true, corrupt the head. Otherwise, corrupt the tail.
void EmbeddingEvaluation::evalCorruption(int head, int tail, int relation, bool corruptHead,
                                   int* rawSumActualRank, int* filteredSumActualRank,
                                   int* rawHitsIn10, int* filteredHitsIn10) {
   // The energies of all permutation of this triple.
   // The first in the pair is what we are using to corrupt.
   std::vector<std::pair<int, double>> tripleEnergies;

   // Recall that we want to do self replacement as well, ie. we want the score for the real triple.
   for (int i = 0; i < numEntities_; i++) {
      if (corruptHead) {
         tripleEnergies.push_back(std::make_pair(i, tripleEnergy(i, tail, relation)));
      } else {
         tripleEnergies.push_back(std::make_pair(i, tripleEnergy(head, i, relation)));
      }
   }
   // Sort in ascending order by energy.
   sort(tripleEnergies.begin(), tripleEnergies.end(), cmp);

   int rawRank = 0;
   int filteredRank = 0;

   for (int i = 0; i < tripleEnergies.size(); i++) {
      int currentHead;
      int currentTail;

      if (corruptHead) {
         currentHead = tripleEnergies[i].first;
         currentTail = tail;
      } else {
         currentHead = head;
         currentTail = tripleEnergies[i].first;
      }

      if (triples_[std::make_pair(currentHead, relation)].count(currentTail) == 0) {
         filteredRank++;
      }

      if (currentHead == head && currentTail == tail) {
         // Note that we start our ranks at 1, not zero.
         rawRank = i + 1;
         filteredRank++;
         break;
      }
   }

   assert(rawRank > 0);
   assert(filteredRank > 0);

   *rawSumActualRank += rawRank;
   *filteredSumActualRank += filteredRank;

   if (rawRank <= 10) {
      (*rawHitsIn10)++;
   }

   if (filteredRank <= 10) {
      (*filteredHitsIn10)++;
   }
}

void EmbeddingEvaluation::run() {
   // Note that we are keeping two sets of stats.
   //  Raw: Consider any triple not the original as bad.
   //  Filtered: Ignore triples that are known good, don't consider them bad.

   // The number of test triples ranked in the top ten.
   int rawHitsIn10 = 0;
   int filteredHitsIn10 = 0;

   // The sum of the ranks the real triples were put at.
   int rawSumActualRank = 0;
   int filteredSumActualRank = 0;

   // For each triple in our working set.
   for (int tripleId = 0; tripleId < heads_.size(); tripleId++) {
      int head = heads_[tripleId];
      int tail = tails_[tripleId];
      int relation = relations_[tripleId];

      evalCorruption(head, tail, relation, true,
                     &rawSumActualRank, &filteredSumActualRank,
                     &rawHitsIn10, &filteredHitsIn10);

      evalCorruption(head, tail, relation, false,
                     &rawSumActualRank, &filteredSumActualRank,
                     &rawHitsIn10, &filteredHitsIn10);
   }

   double numberCorruptions = heads_.size() * 2;

   printf("Raw      -- Rank: %f, Hits@10: %f\n", rawSumActualRank / numberCorruptions, rawHitsIn10 / numberCorruptions);
   printf("Filtered -- Rank: %f, Hits@10: %f\n", filteredSumActualRank / numberCorruptions, filteredHitsIn10 / numberCorruptions);
}

void EmbeddingEvaluation::prepare() {
   if (!fileExists(relationEmbeddingPath_)) {
      printf("Could not find relation embedding file: %s. Make sure to specify the path and/or train.\n", relationEmbeddingPath_);
      exit(2);
   }

   if (!fileExists(entityEmbeddingPath_)) {
      printf("Could not find entity embedding file: %s. Make sure to specify the path and/or train.\n", entityEmbeddingPath_);
      exit(2);
   }

   loadTriples();
   loadEmbeddings();
}

}  // namespace common
