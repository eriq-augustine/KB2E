#include <iostream>
#include <string>

#include "common/args.h"
#include "common/evaluation.h"
#include "common/utils.h"
#include "transe/evaluation.h"

int main(int argc, char** argv) {
    common::EmbeddingArguments args = common::parseArgs(argc, argv);
    printf("%s\n", args.to_string().c_str());

    srand(args.seed);

    common::EmbeddingEvaluation* evaluation = new transe::Evaluation(args);
    evaluation->prepare();
    evaluation->run();
    delete(evaluation);
}
