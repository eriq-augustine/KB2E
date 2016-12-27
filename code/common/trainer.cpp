#include "common/trainer.h"

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

#define ID_STRING_MAX_LEN 512

// TODO(eriq): Config
#define L1_FLAG true

namespace common {

const std::string Trainer::DATA_DIR = "../data";
const std::string Trainer::ENTITY_ID_FILE = "entity2id.txt";
const std::string Trainer::RELATION_ID_FILE = "relation2id.txt";
const std::string Trainer::TRAIN_FILE = "train.txt";

void Trainer::add(int x, int y, int z) {
   fb_h.push_back(x);
   fb_r.push_back(z);
   fb_l.push_back(y);

   ok[std::make_pair(x, z)][y] = 1;
}

void Trainer::train() {
   relation_vec_.resize(numRelations_);
   for (int i = 0; i <relation_vec_.size(); i++) {
      relation_vec_[i].resize(n_);
   }

   entity_vec_.resize(numEntities_);
   for (int i = 0; i <entity_vec_.size(); i++) {
      entity_vec_[i].resize(n_);
   }

   relation_tmp_.resize(numRelations_);
   for (int i = 0; i <relation_tmp_.size(); i++) {
      relation_tmp_[i].resize(n_);
   }

   entity_tmp_.resize(numEntities_);
   for (int i = 0; i <entity_tmp_.size(); i++) {
      entity_tmp_[i].resize(n_);
   }

   for (int i = 0; i <numRelations_; i++) {
      for (int ii = 0; ii <n_; ii++) {
         relation_vec_[i][ii] = randn(0,1.0/n_,-6/sqrt(n_),6/sqrt(n_));
      }
   }

   for (int i = 0; i <numEntities_; i++) {
      for (int ii = 0; ii <n_; ii++) {
         entity_vec_[i][ii] = randn(0,1.0/n_,-6/sqrt(n_),6/sqrt(n_));
      }
      norm(entity_vec_[i]);
   }

   bfgs();
}

std::string Trainer::methodName() {
    if (method_) {
        return "bern";
    }

    return "unif";
}

void Trainer::bfgs() {
   lossValue_ = 0;

   // TODO(eriq): Config
   int numBatches = 100;
   int maxEpochs = 1000;

   int batchsize = fb_h.size() / numBatches;

   for (int epoch = 0; epoch < maxEpochs; epoch++) {

      lossValue_=0;
      for (int batch = 0; batch<numBatches; batch++)
      {
         relation_tmp_=relation_vec_;
         entity_tmp_ = entity_vec_;
         for (int k=0; k<batchsize; k++)
         {
            int i=randMax(fb_h.size());
            int j=randMax(numEntities_);
            double pr = 1000*right_num[fb_r[i]]/(right_num[fb_r[i]]+left_num[fb_r[i]]);
            if (method_ ==0) {
               pr = 500;
            }

            if (std::rand() % 1000 < pr) {
               while (ok[std::make_pair(fb_h[i],fb_r[i])].count(j)>0) {
                  j = randMax(numEntities_);
               }
               train_kb(fb_h[i],fb_l[i],fb_r[i],fb_h[i],j,fb_r[i]);
            } else {
               while (ok[std::make_pair(j, fb_r[i])].count(fb_l[i]) > 0) {
                  j = randMax(numEntities_);
               }
               train_kb(fb_h[i],fb_l[i],fb_r[i],j,fb_l[i],fb_r[i]);
            }

            norm(relation_tmp_[fb_r[i]]);
            norm(entity_tmp_[fb_h[i]]);
            norm(entity_tmp_[fb_l[i]]);
            norm(entity_tmp_[j]);
         }
         relation_vec_ = relation_tmp_;
         entity_vec_ = entity_tmp_;
      }
      std::cout<<"epoch:"<<epoch<<' '<<lossValue_<<std::endl;
      FILE* f2 = fopen(("relation2vec." + methodName()).c_str(),"w");
      FILE* f3 = fopen(("entity2vec." + methodName()).c_str(),"w");
      for (int i = 0; i <numRelations_; i++)
      {
         for (int ii = 0; ii <n_; ii++)
               fprintf(f2,"%.6lf\t",relation_vec_[i][ii]);
         fprintf(f2,"\n");
      }
      for (int i = 0; i <numEntities_; i++)
      {
         for (int ii = 0; ii <n_; ii++)
               fprintf(f3,"%.6lf\t",entity_vec_[i][ii]);
         fprintf(f3,"\n");
      }
      fclose(f2);
      fclose(f3);
   }
}

double Trainer::calc_sum(int e1,int e2,int rel) {
   double sum=0;
   if (L1_FLAG)
         for (int ii = 0; ii <n_; ii++)
            sum+=fabs(entity_vec_[e2][ii]-entity_vec_[e1][ii]-relation_vec_[rel][ii]);
   else
         for (int ii = 0; ii <n_; ii++)
            sum+=sqr(entity_vec_[e2][ii]-entity_vec_[e1][ii]-relation_vec_[rel][ii]);
   return sum;
}

void Trainer::gradient(int e1_a,int e2_a,int rel_a,int e1_b,int e2_b,int rel_b) {
   for (int ii = 0; ii <n_; ii++) {

         double x = 2*(entity_vec_[e2_a][ii]-entity_vec_[e1_a][ii]-relation_vec_[rel_a][ii]);
         if (L1_FLAG)
            if (x>0)
               x=1;
            else
               x=-1;
         relation_tmp_[rel_a][ii]-=-1*learningRate_*x;
         entity_tmp_[e1_a][ii]-=-1*learningRate_*x;
         entity_tmp_[e2_a][ii]+=-1*learningRate_*x;
         x = 2*(entity_vec_[e2_b][ii]-entity_vec_[e1_b][ii]-relation_vec_[rel_b][ii]);
         if (L1_FLAG)
            if (x>0)
               x=1;
            else
               x=-1;
         relation_tmp_[rel_b][ii]-=learningRate_*x;
         entity_tmp_[e1_b][ii]-=learningRate_*x;
         entity_tmp_[e2_b][ii]+=learningRate_*x;
   }
}

void Trainer::train_kb(int e1_a,int e2_a,int rel_a,int e1_b,int e2_b,int rel_b) {
   double sum1 = calc_sum(e1_a,e2_a,rel_a);
   double sum2 = calc_sum(e1_b,e2_b,rel_b);
   if (sum1+margin_>sum2)
   {
         lossValue_+=margin_+sum1-sum2;
         gradient( e1_a, e2_a, rel_a, e1_b, e2_b, rel_b);
   }
}

void Trainer::loadFiles() {
    std::map<std::string, int> entity2id;
    std::map<std::string, int> relation2id;

    std::map<int, std::map<int, int>> left_entity;
    std::map<int, std::map<int, int>> right_entity;

    int entityId;
    char headIdStringBuf[ID_STRING_MAX_LEN];

    // TODO(eriq): Better error handling with files.

    FILE* entityIdFile = fopen((DATA_DIR + "/" + ENTITY_ID_FILE).c_str(), "r");
    while (fscanf(entityIdFile, "%s\t%d", headIdStringBuf, &entityId) == 2) {
        std::string entityIdString = headIdStringBuf;
        entity2id[entityIdString] = entityId;
    }
    fclose(entityIdFile);

    FILE* relationIdFile = fopen((DATA_DIR + "/" + RELATION_ID_FILE).c_str(), "r");
    while (fscanf(relationIdFile, "%s\t%d", headIdStringBuf, &entityId) == 2) {
        std::string entityIdString = headIdStringBuf;
        relation2id[entityIdString] = entityId;
    }
    fclose(relationIdFile);

    char tailIdStringBuf[ID_STRING_MAX_LEN];
    char relationIdStringBuf[ID_STRING_MAX_LEN];

    FILE* trainFile = fopen((DATA_DIR + "/" + TRAIN_FILE).c_str(), "r");
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

        left_entity[relation2id[relationIdString]][entity2id[headIdString]]++;
        right_entity[relation2id[relationIdString]][entity2id[tailIdString]]++;
        add(entity2id[headIdString], entity2id[tailIdString], relation2id[relationIdString]);
    }
    fclose(trainFile);

    for (int i = 0; i < relation2id.size(); i++) {
        double sum1 = 0;
        double sum2 = 0;

        for (std::map<int, int>::iterator it = left_entity[i].begin(); it != left_entity[i].end(); it++) {
            sum1++;
            sum2 += it->second;
        }

        left_num[i] = sum2 / sum1;
    }

    for (int i = 0; i < relation2id.size(); i++) {
        double sum1=0,sum2=0;

        for (std::map<int, int>::iterator it = right_entity[i].begin(); it != right_entity[i].end(); it++) {
            sum1++;
            sum2 += it->second;
        }

        right_num[i] = sum2 / sum1;
    }

    numRelations_ = relation2id.size();
    numEntities_ = entity2id.size();

    std::cout << "Number of Relations: " << relation2id.size() << std::endl;
    std::cout << "Number of Entities: " << entity2id.size() << std::endl;
}

// TODO(eriq)
/*
int main(int argc, char**argv) {
    srand((unsigned) time(NULL));
    int method = 1;
    int n_ = 100;
    double rate = 0.001;
    double margin = 1;
    int i;

    if ((i = argpos((char *)"-size", argc, argv)) > 0) n_ = atoi(argv[i + 1]);
    if ((i = argpos((char *)"-margin", argc, argv)) > 0) margin = atoi(argv[i + 1]);
    if ((i = argpos((char *)"-rate", argc, argv)) > 0) rate = atof(argv[i + 1]);
    if ((i = argpos((char *)"-method", argc, argv)) > 0) method = atoi(argv[i + 1]);

    if (method) {
        version = "bern";
    } else {
        version = "unif";
    }

    std::cout << "size = " << n_ << std::endl;
    std::cout << "learing rate = " << rate << std::endl;
    std::cout << "margin = " << margin << std::endl;
    std::cout << "method = " << version << std::endl;

    prepare();
    trainer.train(n_, rate, margin, method);
}
*/

}  // namespace common
