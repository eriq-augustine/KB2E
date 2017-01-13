import os,sys
import math
import random
import time 

# TODO(eriq):
#  - Change string concatenation to pair.
#  - Paths
#  - Files

# This is to measure the reliability of a path.

HEAD = 0
TAIL = 1
RELATION = 2

def map_add(mp, key1, key2, value):
   if (key1 not in mp):
      mp[key1] = {}
   if (key2 not in mp[key1]):
      mp[key1][key2] = 0.0
   mp[key1][key2] += value
   

def map_add1(mp, key):
   if (key not in mp):
      mp[key] = 0
   mp[key] += 1

# Operates on triples.
def work(basename):
   inFile = open("data/" + basename + ".txt","r")
   outFile = open("data/" + basename + "_pra.txt","w")

   for line in inFile:
      triple = line.strip().split()
      head = triple[HEAD]
      tail = triple[TAIL]
      relation = relation2id[triple[RELATION]]

      outFile.write(str(head)+" "+str(tail)+' '+str(relation)+"\n")
      b = {}
      a = {}

      if (head+' '+tail in h_e_p):
         sum = 0.0
         for rel_path in h_e_p[head+' '+tail]:
            b[rel_path] = h_e_p[head+' '+tail][rel_path]
            sum += b[rel_path]
         for rel_path in b:
            b[rel_path]/=sum
            if b[rel_path]>0.01:
               a[rel_path] = b[rel_path]
      outFile.write(str(len(a)))
      for rel_path in a:
         outFile.write(" "+str(len(rel_path.split()))+" "+rel_path+" "+str(a[rel_path]))
      outFile.write("\n")
      outFile.write(str(tail)+" "+str(head)+' '+str(relation+len(relation2id))+"\n")
      head = triple[TAIL]
      tail = triple[HEAD]
      b = {}
      a = {}
      if (head+' '+tail in h_e_p):
         sum = 0.0
         for rel_path in h_e_p[head+' '+tail]:
            b[rel_path] = h_e_p[head+' '+tail][rel_path]
            sum += b[rel_path]
         for rel_path in b:
            b[rel_path]/=sum
            if b[rel_path]>0.01:
               a[rel_path] = b[rel_path]
      outFile.write(str(len(a)))
      for rel_path in a:
         outFile.write(" "+str(len(rel_path.split()))+" "+rel_path+" "+str(a[rel_path]))
      outFile.write("\n")

   inFile.close()
   outFile.close()

def fetchRelations():
    inFile = open("data/relation2id.txt","r")
    relation2id = {}

    for line in inFile:
        parts = line.strip().split()
        relation2id[parts[0]] = int(parts[1])

    inFile.close()
    return relation2id

def loadTrain(entityRelationMap, triples):
   inFile = open("data/train.txt","r")
   for line in inFile:
      seg = line.strip().split()
      head = seg[HEAD]
      tail = seg[TAIL]
      relation = seg[RELATION]

      if (head+" "+tail not in entityRelationMap):
         entityRelationMap[head+" "+tail] = {}

      entityRelationMap[head+" "+tail][relation2id[relation]] = 1

      if (tail+" "+head not in entityRelationMap):
         entityRelationMap[tail+" "+head] = {}

      # TODO(eriq): Check very carefully about this.
      #  They are using additional relation ids past the end.
      entityRelationMap[tail+" "+head][relation2id[relation]+len(relation2id)] = 1

      if (head not in triples):
         triples[head] = {}

      if (relation2id[relation] not in triples[head]):
         triples[head][relation2id[relation]] = {}

      triples[head][relation2id[relation]][tail] = 1

      if (tail not in triples):
         triples[tail] = {}

      if ((relation2id[relation]+len(relation2id)) not in triples[tail]):
         triples[tail][relation2id[relation]+len(relation2id)] = {}

      triples[tail][relation2id[relation]+len(relation2id)][head] = 1

   inFile.close()

def loadTest(entityRelationMap):
   inFile = open("data/test.txt","r")
   for line in inFile:
      seg = line.strip().split()

      if (seg[HEAD]+" "+seg[TAIL] not in entityRelationMap):
         entityRelationMap[seg[HEAD]+' '+seg[TAIL]]={}

      if (seg[TAIL]+" "+seg[HEAD] not in entityRelationMap):
         entityRelationMap[seg[TAIL]+' '+seg[HEAD]]={}

   inFile.close()

# TODO(eriq): This overrides.
def loadTopEntityPairs(entityRelationMap):
   inFile = open("data/e1_e2.txt","r")
   for line in inFile:
      seg = line.strip().split()
      entityRelationMap[seg[HEAD]+" "+seg[TAIL]] = {}
      entityRelationMap[seg[TAIL]+" "+seg[HEAD]] = {}
   inFile.close()


#


relation2id = fetchRelations()

# {head+" "+tail: {relationId: 1, ...}, ...}
# TODO(eriq): For each relation, an additional key was put in that was its index + the number of relations.
entityRelationMap = {}

# {head: {relationId: {tail: 1, ...}, ...}, ...}
# {tail: {(relationId + len(relations)): {head: 1, ...}, ...}, ...}
triples = {}

loadTrain(entityRelationMap, triples)
loadTest(entityRelationMap)
# TODO(eriq): This overrides.
loadTopEntityPairs(entityRelationMap)



g = open("data/path2.txt","w")


path_dict = {}
path_r_dict = {}
train_path = {}


step = 0
time1= time.time()
path_num = 0

h_e_p={}
for head in triples:
   step+=1
   print step,
   for rel1 in triples[head]:
      e2_set = triples[head][rel1]
      for tail in e2_set:
         map_add1(path_dict,str(rel1))
         for key in entityRelationMap[head+' '+tail]:
            map_add1(path_r_dict,str(rel1)+"->"+str(key))
         map_add(h_e_p,head+' '+tail,str(rel1),1.0/len(e2_set))
   for rel1 in triples[head]:
      e2_set = triples[head][rel1]
      for tail in e2_set:
         if (tail in triples):
            for rel2 in triples[tail]:
               e3_set = triples[tail][rel2]
               for e3 in e3_set:
                  map_add1(path_dict,str(rel1)+" "+str(rel2))
                  if (head+" "+e3 in entityRelationMap):
                     for key in entityRelationMap[head+' '+e3]:
                        map_add1(path_r_dict,str(rel1)+" "+str(rel2)+"->"+str(key))
                  if (head+" "+e3 in entityRelationMap):# and h_e_p[head+' '+tail][str(rel1)]*1.0/len(e3_set)>0.01):
                     map_add(h_e_p,head+' '+e3,str(rel1)+' '+str(rel2),h_e_p[head+' '+tail][str(rel1)]*1.0/len(e3_set))
   for tail in triples:
      e_1 = head
      e_2 = tail
      if (e_1+" "+e_2 in h_e_p):
         path_num+=len(h_e_p[e_1+" "+e_2])
         bb = {}
         aa = {}
         g.write(str(e_1)+" "+str(e_2)+"\n")
         sum = 0.0
         for rel_path in h_e_p[e_1+' '+e_2]:
            bb[rel_path] = h_e_p[e_1+' '+e_2][rel_path]
            sum += bb[rel_path]
         for rel_path in bb:
            bb[rel_path]/=sum
            if bb[rel_path]>0.01:
               aa[rel_path] = bb[rel_path]
         g.write(str(len(aa)))
         for rel_path in aa:
            train_path[rel_path] = 1
            g.write(" "+str(len(rel_path.split()))+" "+rel_path+" "+str(aa[rel_path]))
         g.write("\n")
   print path_num, time.time()-time1
   sys.stdout.flush()
g.close()

g = open("data/confidence.txt","w")
for rel_path in train_path:
   
   out = []
   for i in range(0,len(relation2id)):
      if (rel_path in path_dict and rel_path+"->"+str(i) in path_r_dict):
         out.append(" "+str(i)+" "+str(path_r_dict[rel_path+"->"+str(i)]*1.0/path_dict[rel_path]))
   if (len(out)>0):
      g.write(str(len(rel_path.split()))+" "+rel_path+"\n")
      g.write(str(len(out)))
      for i in range(0,len(out)):
         g.write(out[i])
      g.write("\n")
g.close()

work("train")
work("test")
