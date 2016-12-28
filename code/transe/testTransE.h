#ifndef TRANSE_TESTTRANSE_H_
#define TRANSE_TESTTRANSE_H_

#include <map>
#include <vector>

namespace transe {

class EmbeddingTest {
   public:
      explicit EmbeddingTest(int method) : method_(method) {}
      void prepare();
      void run();

   private:
      // TODO TEST
      int embeddingSize_ = 100;
      std::string outputDir_ = ".";

      std::vector<std::vector<double>> relationVec_;
      std::vector<std::vector<double>> entityVec_;

      int numRelations_;
      int numEntities_;

      int method_;

      std::vector<int> heads_;
      std::vector<int> tails_;
      std::vector<int> relations_;

      // {<head, relation>: {tail: 1}}
      std::map<std::pair<int, int>, std::map<int, int>> triples_;

      void add(int head, int tail, int relation, bool addToWorkingSet);

      double tripleEnergy(int head, int tail, int relation);
      void loadEmbeddings();
      void evalCorruption(int head, int tail, int relation, bool corruptHead,
                          int* rawSumActualRank, int* filteredSumActualRank,
                          int* rawHitsIn10, int* filteredHitsIn10);
};

}  // namespace transe

#endif  // TRANSE_TESTTRANSE_H_
