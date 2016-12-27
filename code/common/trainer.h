#ifndef COMMON_TRAINER_H_
#define COMMON_TRAINER_H_

#include <map>
#include <vector>
#include <string>
#include <utility>

namespace common {

class Trainer {
   public:
      std::string dataDir_;
      std::string outputDir_;

      explicit Trainer(std::string dataDir, std::string outputDir,
                       int embeddingSize, double learningRate, double margin,
                       int method, int numBatches, int maxEpochs);

      void add(int head, int tail, int relation);
      void loadFiles();
      void train();

      virtual void write();

   protected:
      int embeddingSize_;
      int method_;
      double learningRate_;
      double margin_;

      std::vector<std::vector<double>> relation_vec_;
      std::vector<std::vector<double>> entity_vec_;

      int numRelations_;
      int numEntities_;

      int numBatches_;
      int maxEpochs_;

      // <relation, frequency>
      // The mean co-occurance count of this relation with entities that appear in the head.
      std::map<int,double> relationHeadMeanCooccurrence_;
      // The mean co-occurance count of this relation with entities that appear in the tail.
      std::map<int,double> relationTailMeanCooccurrence_;

      // The triples separated out into three vectors.
      std::vector<int> heads_;
      std::vector<int> tails_;
      std::vector<int> relations_;

      // {<head, relation>: {tail: 1}}
      std::map<std::pair<int, int>, std::map<int, int> > triples_;

      //

      void bfgs();
      double train_kb(int e1_a,int e2_a,int rel_a,int e1_b,int e2_b,int rel_b);
      std::string methodName();

      // The first value to put in all the embedding vectors.
      virtual double initialEmbeddingValue() = 0;

      virtual void gradientUpdate(int head, int tail, int relation, bool corrupted) = 0;

      // Called after a batch has just finished.
      virtual void postbatch() {};

      // Called before a new batch is started.
      virtual void prebatch() {};

      // Called as the first action in train().
      virtual void prepTrain();

      virtual double tripleEnergy(int head, int tail, int relation) = 0;
};

}  // namespace common

#endif  // COMMON_TRAINER_H_
