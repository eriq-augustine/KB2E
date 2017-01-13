#ifndef PTRANSE_TRAINER_H_
#define PTRANSE_TRAINER_H_

#include <vector>

#include "common/args.h"
#include "common/trainer.h"
#include "common/utils.h"

namespace ptranse {

class Trainer : public common::Trainer {
   public:
      explicit Trainer(common::EmbeddingArguments args);

   protected:
      // The next values for the embeddings.
      std::vector<std::vector<double>> relationVec_next_;
      std::vector<std::vector<double>> entityVec_next_;

      std::vector<std::vector<std::pair<std::vector<int>, double>>> paths_;
      std::map<std::pair<std::string, int>, double> pathConfidence_;

      int distanceType_;

      void add(int head, int tail, int relation, std::vector<std::pair<std::vector<int>, double>> path);

      double initialEmbeddingValue() override;
      void gradientUpdate(int head, int tail, int relation, bool corrupted) override;
      void postbatch() override;
      void prebatch() override;
      double tripleEnergy(int head, int tail, int relation) override;

      void bfgs() override;
};

}  // namespace ptranse

#endif  // PTRANSE_TRAINER_H_

////

#include <iostream>
#include <cstring>
#include <cstdio>
#include <map>
#include <vector>
#include <string>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <sstream>

void Trainer::add(int head, int tail, int relation, std::vector<std::pair<std::vector<int>, double>> path) {
   add(head, tail, relation);
   paths_.push_back(path);
}

double Trainer::initialEmbeddingValue() {
   return common::randn(0, 1.0 / embeddingSize_, -6 / std::sqrt(embeddingSize_), 6 / std::sqrt(embeddingSize_));
}

void Trainer::postbatch() {
   relationVec_ = relationVec_next_;
   entityVec_ = entityVec_next_;
}

void Trainer::prebatch() {
   relationVec_next_ = relationVec_;
   entityVec_next_ = entityVec_;
}

double Trainer::tripleEnergy(int head, int tail, int relation) {
   double energy = 0;

   for (int i = 0; i < embeddingSize_; i++) {
      double tmp = entityVec_[tail][i] - entityVec_[head][i] - relationVec_[relation][i];

      if (distanceType_ == L1_DISTANCE) {
         energy += std::fabs(tmp);
      } else {
         energy += common::sqr(tmp);
      }
   }

   return energy;
}

void Trainer::gradientUpdate(int head, int tail, int relation, bool corrupted) {
   double modifier = corrupted ? 1.0 : -1.0;

   for (int i = 0; i < embeddingSize_; i++) {
      double x = 2 * (entityVec_[tail][i] - entityVec_[head][i] - relationVec_[relation][i]);

      if (distanceType_ == L1_DISTANCE) {
         if (x > 0) {
            x = 1;
         } else {
            x =- 1;
         }
      }

      relationVec_next_[relation][i] -= modifier * learningRate_ * x;
      entityVec_next_[head][i] -= modifier * learningRate_ * x;
      entityVec_next_[tail][i] += modifier * learningRate_ * x;
   }
}

// TODO(eriq): Some custom at the end of every batch?
void train_path(int relation, int rel_neg, std::vector<int> rel_path, double margin, double x) {
   double loss = 0;

   double sum1 = calc_path(relation, rel_path);
   double sum2 = calc_path(rel_neg, rel_path);
   // TODO(eriq): Param?
   double lambda = 1;

   if (sum1 + margin > sum2) {
      loss = x * lambda * (margin + sum1 - sum2);
      gradient_path(relation, rel_path, -x * lambda);
      gradient_path(rel_neg, rel_path, x * lambda);
   }

   return loss;
}

double calc_path(int r1, std::vector<int> rel_path) {
   double sum = 0;

   for (int i = 0; i < embeddingSize_; i++) {
      double tmp = relationVec_[r1][i];
      for (int j = 0; j < rel_path.size(); j++) {
         tmp -= relationVec_[rel_path[j]][i];
      }

      if (distanceType_ == L1_DISTANCE) {
         sum += fabs(tmp);
      } else {
         sum += common::sqr(tmp);
      }
   }

   return sum;
}

void gradient_path(int r1, std::vector<int> rel_path, double belta) {
   for (int i = 0; i < embeddingSize_; i++) {
      double x = relationVec_[r1][i];

      for (int j = 0; j < rel_path.size(); j++) {
         x -= relationVec_[rel_path[j]][i];
      }

      if (distanceType_ == L1_DISTANCE) {
         if (x > 0) {
            x = 1;
         } else {
            x =- 1;
         }
      }

      relationVec_next_[r1][i] += belta * learningRate_ * x;
      for (int j = 0; j < rel_path.size(); j++) {
         relationVec_next_[rel_path[j]][i] -= belta * learningRate_ * x;
      }
   }
}

void Trainer::bfgs() {
   int batchsize = heads_.size() / numBatches_;

   // TODO(eriq): TEST
   std::map<std::vector<int>, string> path2s;

   for (int epoch = 0; epoch < maxEpochs_; epoch++) {
      double loss=0;

      for (int batch = 0; batch < numBatches_; batch++) {
         prebatch();

         for (int k = 0; k < batchsize; k++) {
            int i = common::randMax(heads_.size());
            int j = common::randMax(numEntities_);

            int head = heads_[i];
            int relation = relations_[i];
            int tail = tails_[i];
            
            int randVal = std::rand() % 100;

            if (randVal < 25) {
               while (triples_[make_pair(head, relation)].count(j) > 0) {
                  j = common::randMax(numEntities_);
               }
               loss += train_kb(head, tail, relation, head, j, relation);
            } else {
               if (randVal < 50) {
                  while (triples_[make_pair(j, relation)].count(tail) > 0) {
                     j = common::randMax(numEntities_);
                  }
                  loss += train_kb(head, tail, relation, j, tail, relation);
               } else {
                  int rel_neg = common::randMax(numRelations_);
                  while (triples_[make_pair(head, rel_neg)].count(tail) > 0) {
                     rel_neg = common::randMax(numRelations_);
                  }
                  loss += train_kb(head, tail, relation, head, tail, rel_neg);
               }

               if (paths_[i].size() > 0) {
                  int rel_neg = common::randMax(numRelations_);
                  while (triples_[make_pair(head, rel_neg)].count(tail) > 0) {
                     rel_neg = common::randMax(numRelations_);
                  }

                  for (int path_id = 0; path_id < paths_[i].size(); path_id++) {
                     std::vector<int> rel_path = paths_[i][path_id].first;
                     std::string s = "";

                     if (path2s.count(rel_path) == 0) {
                        ostringstream oss;  // Build one piece at a time.
                        for (int j = 0; j < rel_path.size(); j++) {
                           oss << rel_path[j] << " ";
                        }
                        s = oss.str();
                        path2s[rel_path] = s;
                     }
                     s = path2s[rel_path];

                     double pr = paths_[i][path_id].second;
                     double pr_path = 0;
                     if (pathConfidence_.count(make_pair(s,relation)) > 0) {
                        pr_path = pathConfidence_[make_pair(s,relation)];
                     }

                     pr_path = 0.99 * pr_path + 0.01;
                     loss += train_path(relation, rel_neg, rel_path, 2 * margin_, pr * pr_path);
                  }
               }
            }

            common::norm(relationVec_next_[relation], false);
            common::norm(entityVec_next_[head], false);
            common::norm(entityVec_next_[tail], false);
            common::norm(entityVec_next_[j], false);
         }
      }

      // TODO(eriq): Debug
      printf("Epoch: %d, Loss: %f\n", epoch, loss);
   }
}






///
//
//


void prepare() {
   // TODO(eriq): Figure out this file format.
   // Will require redoing the python script.
   int x;
   FILE* f_kb = fopen("../data/train_pra.txt","r");
   while (fscanf(f_kb,"%s",buf)==1)
   {
      string s1=buf;
      fscanf(f_kb,"%s",buf);
      string s2=buf;
      if (entity2id.count(s1)==0)
      {
         cout<<"miss entity:"<<s1<<endl;
      }
      if (entity2id.count(s2)==0)
      {
         cout<<"miss entity:"<<s2<<endl;
      }
      int head = entity2id[s1];
      int tail = entity2id[s2];
      int relation;
      fscanf(f_kb,"%d",&relation);
      fscanf(f_kb,"%d",&x);
      std::vector<std::pair<std::vector<int>,double> > b;
      b.clear();
      for (int i = 0; i<x; i++)
      {
         int y,z;
         std::vector<int> rel_path;
         rel_path.clear();
         fscanf(f_kb,"%d",&y);
         for (int j=0; j<y; j++)
         {
            fscanf(f_kb,"%d",&z);
            rel_path.push_back(z);
         }
         double pr;
         fscanf(f_kb,"%lf",&pr);
         b.push_back(make_pair(rel_path,pr));
      }
      //cout<<head<<' '<<tail<<' '<<relation<<' '<<b.size()<<endl;
      train.add(head,tail,relation,b);
   }

   // TODO(eriq): WTF?
   numRelations_ *= 2;
   
   cout<<"numRelations_="<<numRelations_<<endl;
   cout<<"numEntities_="<<numEntities_<<endl;
   
   FILE* f_confidence = fopen("../data/confidence.txt","r");
   while (fscanf(f_confidence,"%d",&x)==1)
   {
      string s = "";
      for (int i=0; i<x; i++)
      {
         fscanf(f_confidence,"%s",buf);
         s = s + string(buf)+" ";
      }
      fscanf(f_confidence,"%d",&x);
      for (int i=0; i<x; i++)
      {
         int y;
         double pr;
         fscanf(f_confidence,"%d%lf",&y,&pr);
      //   cout<<s<<' '<<y<<' '<<pr<<endl;
         pathConfidence_[make_pair(s,y)] = pr;
      }
   }
   fclose(f_confidence);
   fclose(f_kb);
}
