#ifndef COMMON_EVALUATION_H_
#define COMMON_EVALUATION_H_

#include <map>
#include <vector>

#include "common/args.h"
#include "common/utils.h"

namespace common {

class EmbeddingEvaluation {
   public:
      explicit EmbeddingEvaluation(EmbeddingArguments args);
      ~EmbeddingEvaluation();

      void prepare();
      void run();

   protected:
      int embeddingSize_;
      std::string dataDir_;
      std::string outputDir_;
      int method_;

      // To speed up evaluation, we will cache the energies for each relation.
      // Experiments have shown that it is no slower for quick calculations like TransE,
      // and about 50% faster for complex calculations like TransR.
      double* energyCache_;

      std::string relationEmbeddingPath_;
      std::string entityEmbeddingPath_;

      std::vector<std::vector<double>> relationVec_;
      std::vector<std::vector<double>> entityVec_;

      int numRelations_;
      int numEntities_;

      std::vector<int> heads_;
      std::vector<int> tails_;
      std::vector<int> relations_;

      // {<head, relation>: {tail: 1}}
      std::map<std::pair<int, int>, std::map<int, int>> triples_;

      void add(int head, int tail, int relation, bool addToWorkingSet);
      void evalCorruption(int head, int tail, int relation, bool corruptHead,
                          int* rawSumRank, int* filteredSumRank,
                          int* rawHitsIn10, int* filteredHitsIn10,
                          std::vector<std::pair<int, double> >& tripleEnergies);

      void loadTriples();

      double cachedTripleEnergy(int head, int tail, int relation);

      virtual double tripleEnergy(int head, int tail, int relation) = 0;
      virtual void loadEmbeddings();
};

}  // namespace common

#endif  // COMMON_EVALUATION_H_
