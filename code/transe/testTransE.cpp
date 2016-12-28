#include "transe/testTransE.h"

#include <functional>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "common/constants.h"
#include "common/loader.h"
#include "common/utils.h"
#include "transe/transe.h"

namespace transe {

bool L1_flag = true;

double cmp(std::pair<int, double> a, std::pair<int, double> b) {
   return a.second < b.second;
}

void EmbeddingTest::prepare() {
   std::map<std::string, int> relation2id;
   std::map<std::string, int> entity2id;

   common::loadIdFile("../data/entity2id.txt", entity2id);
   common::loadIdFile("../data/relation2id.txt", relation2id);

   numEntities_ = entity2id.size();
   numRelations_ = relation2id.size();

   std::function<void(int, int, int)> tripleCallbackTrue = [this](int head, int tail, int relation) {
      this->add(head, tail, relation, true);
   };

   std::function<void(int, int, int)> tripleCallbackFalse = [this](int head, int tail, int relation) {
      this->add(head, tail, relation, false);
   };

   common::loadTripleFile("../data/test.txt", entity2id, relation2id, tripleCallbackTrue);
   common::loadTripleFile("../data/train.txt", entity2id, relation2id, tripleCallbackFalse);
   common::loadTripleFile("../data/valid.txt", entity2id, relation2id, tripleCallbackFalse);
}

void EmbeddingTest::add(int head, int tail, int relation, bool addToWorkingSet) {
   if (addToWorkingSet) {
      heads_.push_back(head);
      relations_.push_back(relation);
      tails_.push_back(tail);
   }

   triples_[std::make_pair(head, relation)][tail] = 1;
}

double EmbeddingTest::tripleEnergy(int head, int tail, int relation) {
   return transe::tripleEnergy(head, tail, relation, embeddingSize_, entityVec_, relationVec_, L1_flag);
}

void EmbeddingTest::run() {
   FILE* f1 = fopen((std::string("relation2vec.") + METHOD_TO_STRING(method_)).c_str(), "r");
   FILE* f3 = fopen((std::string("entity2vec.") + METHOD_TO_STRING(method_)).c_str(), "r");

   std::cout<<numRelations_<<' '<<numEntities_<<std::endl;
   int relation_num_fb=numRelations_;
   relationVec_.resize(relation_num_fb);
   for (int i=0; i<relation_num_fb;i++)
   {
      relationVec_[i].resize(embeddingSize_);
      for (int ii=0; ii<embeddingSize_; ii++)
         fscanf(f1,"%lf",&relationVec_[i][ii]);
   }
   entityVec_.resize(numEntities_);
   for (int i=0; i<numEntities_;i++)
   {
      entityVec_[i].resize(embeddingSize_);
      for (int ii=0; ii<embeddingSize_; ii++)
         fscanf(f3,"%lf",&entityVec_[i][ii]);
      if (common::vec_len(entityVec_[i])-1>1e-3)
         std::cout << "wrong_entity" << i << ' ' << common::vec_len(entityVec_[i]) << std::endl;
   }
   fclose(f1);
   fclose(f3);
   double lsum=0 ,lsum_filter= 0;
   double rsum = 0,rsum_filter=0;
   double lp_n=0,lp_n_filter;
   double rp_n=0,rp_n_filter;
   std::map<int,double> lsum_r,lsum_filter_r;
   std::map<int,double> rsum_r,rsum_filter_r;
   std::map<int,double> lp_n_r,lp_n_filter_r;
   std::map<int,double> rp_n_r,rp_n_filter_r;
   std::map<int,int> rel_num;

   for (int testid = 0; testid<tails_.size(); testid+=1)
   {
      int h = heads_[testid];
      int l = tails_[testid];
      int relation = relations_[testid];
      double tmp = tripleEnergy(h,l,relation);
      rel_num[relation]+=1;
      std::vector<std::pair<int,double> > a;
      for (int i=0; i<numEntities_; i++)
      {
         double sum = tripleEnergy(i,l,relation);
         a.push_back(std::make_pair(i,sum));
      }
      sort(a.begin(),a.end(),cmp);
      double ttt=0;
      int filter = 0;
      for (int i=a.size()-1; i>=0; i--)
      {
         if (triples_[std::make_pair(a[i].first,relation)].count(l)>0)
            ttt++;
         if (triples_[std::make_pair(a[i].first,relation)].count(l)==0)
            filter+=1;
         if (a[i].first ==h)
         {
            lsum+=a.size()-i;
            lsum_filter+=filter+1;
            lsum_r[relation]+=a.size()-i;
            lsum_filter_r[relation]+=filter+1;
            if (a.size()-i<=10)
            {
               lp_n+=1;
               lp_n_r[relation]+=1;
            }
            if (filter<10)
            {
               lp_n_filter+=1;
               lp_n_filter_r[relation]+=1;
            }
            break;
         }
      }
      a.clear();
      for (int i=0; i<numEntities_; i++)
      {
         double sum = tripleEnergy(h,i,relation);
         a.push_back(std::make_pair(i,sum));
      }
      sort(a.begin(),a.end(),cmp);
      ttt=0;
      filter=0;
      for (int i=a.size()-1; i>=0; i--)
      {
         if (triples_[std::make_pair(h,relation)].count(a[i].first)>0)
            ttt++;
         if (triples_[std::make_pair(h,relation)].count(a[i].first)==0)
            filter+=1;
         if (a[i].first==l)
         {
            rsum+=a.size()-i;
            rsum_filter+=filter+1;
            rsum_r[relation]+=a.size()-i;
            rsum_filter_r[relation]+=filter+1;
            if (a.size()-i<=10)
            {
               rp_n+=1;
               rp_n_r[relation]+=1;
            }
            if (filter<10)
            {
               rp_n_filter+=1;
               rp_n_filter_r[relation]+=1;
            }
            break;
         }
      }
   }
   std::cout<<"left:"<<lsum/tails_.size()<<'\t'<<lp_n/tails_.size()<<"\t"<<lsum_filter/tails_.size()<<'\t'<<lp_n_filter/tails_.size()<<std::endl;
   std::cout<<"right:"<<rsum/relations_.size()<<'\t'<<rp_n/relations_.size()<<'\t'<<rsum_filter/relations_.size()<<'\t'<<rp_n_filter/relations_.size()<<std::endl;
}

}  // namespace transe

// TODO(eriq)
int main(int argc, char**argv) {
   if (argc < 2) {
      return 1;
   }

   transe::EmbeddingTest test(atoi(argv[1]));
   test.prepare();
   test.run();
}
