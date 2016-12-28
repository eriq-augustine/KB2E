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
      // TODO
      int embeddingSize_ = 100;

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
};

}  // namespace transe

#endif  // TRANSE_TESTTRANSE_H_
