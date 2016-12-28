#ifndef COMMON_CONSTANTS_H_
#define COMMON_CONSTANTS_H_

#define PI 3.1415926535897932384626433832795

#define ID_STRING_MAX_LEN 512

#define METHOD_UNIF 0
#define METHOD_BERN 1

#define METHOD_NAME_UNIF "unif"
#define METHOD_NAME_BERN "bern"

#define METHOD_TO_STRING(x) (x == METHOD_UNIF ? METHOD_NAME_UNIF : METHOD_NAME_BERN)

#define L1_DISTANCE 0
#define L2_DISTANCE 1

#define ENTITY_ID_FILE "entity2id.txt"
#define RELATION_ID_FILE "relation2id.txt"
#define TRAIN_FILE "train.txt"
#define ENTITY_OUT_FILE_BASENAME "entity2vec"
#define RELATION_OUT_FILE_BASENAME "relation2vec"
#define WEIGHT_OUT_FILE_BASENAME "weights"

#define DEFAULT_EMBEDDING_SIZE 100
#define DEFAULT_LEARNING_RATE 0.001
#define DEFAULT_MARGIN 1.0
#define DEFAULT_METHOD METHOD_BERN
#define DEFAULT_NUM_BATCHES 100
#define DEFAULT_MAX_EPOCHS 1000
#define DEFAULT_DISTANCE L1_DISTANCE

#define DEFAULT_DATA_DIR "../data"
#define DEFAULT_OUTPUT_DIR "."

#define ARG_HELP "help"
#define ARG_DATA_DIR "datadir"
#define ARG_OUT_DIR "outdir"
#define ARG_EMBEDDING_SIZE "size"
#define ARG_LEARNING_RATE "rate"
#define ARG_MARGIN "margin"
#define ARG_METHOD "method"
#define ARG_NUM_BATCHES "batches"
#define ARG_MAX_EPOCHS "epochs"
#define ARG_DISTANCE_TYPE "distance"

#endif  // COMMON_CONSTANTS_H_
