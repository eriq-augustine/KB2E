#ifndef COMMON_TRAINER_H_
#define COMMON_TRAINER_H_

#include <iostream>
#include <cstring>
#include <cstdio>
#include <map>
#include <vector>
#include <string>
#include <ctime>
#include <cmath>
#include <cstdlib>

#include "common/utils.h"

namespace common {

class Trainer {
   public:
      static const std::string DATA_DIR;
      static const std::string ENTITY_ID_FILE;
      static const std::string RELATION_ID_FILE;
      static const std::string TRAIN_FILE;

      explicit Trainer(int n,  double learningRate, double margin, int method)
            : n_(n), learningRate_(learningRate), margin_(margin), method_(method) {}

      void add(int x, int y, int z);
      void loadFiles();
      void train();

   private:
      int n_;
      int method_;
      double learningRate_;
      double margin_;

      double lossValue_;//loss function value

      std::vector<std::vector<double>> relation_vec_;
      std::vector<std::vector<double>> entity_vec_;

      int numRelations_;
      int numEntities_;

      // TODO(eriq): Better name
      std::map<int,double> left_num;
      std::map<int,double> right_num;

      double count;
      double count1;//loss function gradient
      double belta;
      double res1;

      std::vector<int> fb_h,fb_l,fb_r;
      std::vector<std::vector<int> > feature;

      std::vector<std::vector<double>> relation_tmp_;
      std::vector<std::vector<double>> entity_tmp_;

      std::map<std::pair<int,int>, std::map<int,int> > ok;

      //

      void bfgs();
      double calc_sum(int e1,int e2,int rel);
      void gradient(int e1_a,int e2_a,int rel_a,int e1_b,int e2_b,int rel_b);
      void train_kb(int e1_a,int e2_a,int rel_a,int e1_b,int e2_b,int rel_b);

      std::string methodName();
};

}  // namespace common

#endif  // COMMON_TRAINER_H_
