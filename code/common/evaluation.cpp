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

   energyCache_ = NULL;
}

EmbeddingEvaluation::~EmbeddingEvaluation() {
   if (energyCache_ != NULL) {
      free(energyCache_);
   }
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
         if (fscanf(relationEmbeddingFile, "%lf", &relationVec_[i][j]) != 1) {
            printf("Failed to read embedding values from file: '%s'\n", relationEmbeddingPath_.c_str());
            exit(1);
         }
      }
   }
   fclose(relationEmbeddingFile);

   FILE* entityEmbeddingFile = fopen(entityEmbeddingPath_.c_str(), "r");
   entityVec_.resize(numEntities_);
   for (int i = 0; i < numEntities_; i++) {
      entityVec_[i].resize(embeddingSize_);
      for (int j = 0; j < embeddingSize_; j++) {
         if (fscanf(entityEmbeddingFile, "%lf", &entityVec_[i][j]) != 1) {
            printf("Failed to read embedding values from file: '%s'\n", entityEmbeddingPath_.c_str());
            exit(1);
         }
      }

      // TODO(eriq): I don't know what this check it.
      if (vec_len(entityVec_[i]) - 1 > 1e-3) {
         std::cout << "wrong_entity" << i << ' ' << vec_len(entityVec_[i]) << std::endl;
      }
   }
   fclose(entityEmbeddingFile);
}

double EmbeddingEvaluation::cachedTripleEnergy(int head, int tail, int relation) {
   if (energyCache_ != NULL) {
      int cacheIndex = (head * numEntities_) + tail;
      if (energyCache_[cacheIndex] >= 0) {
         return energyCache_[cacheIndex];
      }

      double energy = tripleEnergy(head, tail, relation);
      energyCache_[cacheIndex] = energy;
      return energy;
   }

   return tripleEnergy(head, tail, relation);
}

// If |corruptHead| is true, corrupt the head. Otherwise, corrupt the tail.
// |tripleEnergies| will already be allocated and will probably have old data in it.
void EmbeddingEvaluation::evalCorruption(int head, int tail, int relation, bool corruptHead,
                                         int* rawSumRank, int* filteredSumRank,
                                         int* rawHitsIn10, int* filteredHitsIn10,
                                         std::vector<std::pair<int, double> >& tripleEnergies) {
   // Recall that we want to do self replacement as well, ie. we want the score for the real triple.
   for (int i = 0; i < numEntities_; i++) {
      if (corruptHead) {
         tripleEnergies[i] = std::make_pair(i, cachedTripleEnergy(i, tail, relation));
      } else {
         tripleEnergies[i] = std::make_pair(i, cachedTripleEnergy(head, i, relation));
      }
   }

   // Sort in ascending order by energy.
   sort(tripleEnergies.begin(), tripleEnergies.end(), cmp);

   int rawRank = 1;
   int filteredRank = 1;

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

      if (currentHead == head && currentTail == tail) {
         // Note that we start our ranks at 1, not zero.
         rawRank = i + 1;
         break;
      }

      if (triples_[std::make_pair(currentHead, relation)].count(currentTail) == 0) {
         filteredRank++;
      }
   }

   assert(rawRank > 0);
   assert(filteredRank > 0);

   *rawSumRank += rawRank;
   *filteredSumRank += filteredRank;

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
   int rawSumRank = 0;
   int filteredSumRank = 0;

   // Prep the cache.
   int cacheSize = numEntities_ * numEntities_;
   if (numEntities_ <= MAX_CACHE_ENTITIES) {
      energyCache_ = (double*)calloc(cacheSize, sizeof(double));
   }

   // Allocate the vector that will be cached to avoid reallocation.
   // The energies of all permutation of this triple.
   // The first in the pair is what we are using to corrupt.
   std::vector<std::pair<int, double>> tripleEnergies;
   tripleEnergies.resize(numEntities_);

   int computedCount = 0;

   // To speed up evaluation, we are going to be caching energy results.
   // However, in most cases, there are too many combinations to cache (around 300 billion for FB15k).
   // So, we will do each relation in-turn so we will only need to cache one relation worth at time.
   // This drops FB15k down to about 200 million.
   // We will take a small hit on this inefficient looping structure, but we can optimize later.
   for (int relationId = 0; relationId < numRelations_; relationId++) {
      if (energyCache_ != NULL) {
         for (int i = 0; i < cacheSize; i++) {
            energyCache_[i] = -1;
         }
      }

      // For each triple in our working set.
      for (int tripleId = 0; tripleId < heads_.size(); tripleId++) {
         if (relations_[tripleId] != relationId) {
            continue;
         }

         int head = heads_[tripleId];
         int tail = tails_[tripleId];
         int relation = relations_[tripleId];

         evalCorruption(head, tail, relation, true,
                        &rawSumRank, &filteredSumRank,
                        &rawHitsIn10, &filteredHitsIn10,
                        tripleEnergies);

         evalCorruption(head, tail, relation, false,
                        &rawSumRank, &filteredSumRank,
                        &rawHitsIn10, &filteredHitsIn10,
                        tripleEnergies);

         computedCount++;
      }

      printf("\rProcessed %05.2f%% ...", computedCount * 100.0 / heads_.size());
   }
   printf("\n");

   double numberCorruptions = heads_.size() * 2.0;

   printf("Raw      -- Rank: %f, Hits@10: %f\n", rawSumRank / numberCorruptions, rawHitsIn10 / numberCorruptions);
   printf("Filtered -- Rank: %f, Hits@10: %f\n", filteredSumRank / numberCorruptions, filteredHitsIn10 / numberCorruptions);
}

void EmbeddingEvaluation::prepare() {
   if (!fileExists(relationEmbeddingPath_)) {
      printf("Could not find relation embedding file: %s. Make sure to specify the path and/or train.\n", relationEmbeddingPath_.c_str());
      exit(2);
   }

   if (!fileExists(entityEmbeddingPath_)) {
      printf("Could not find entity embedding file: %s. Make sure to specify the path and/or train.\n", entityEmbeddingPath_.c_str());
      exit(2);
   }

   loadTriples();
   loadEmbeddings();
}

}  // namespace common
