#include <iostream>
#include <string>

#include "common/args.h"
#include "common/trainer.h"
#include "common/utils.h"
#include "transr/trainer.h"

int main(int argc, char** argv) {
    common::EmbeddingArguments args = common::parseArgs(argc, argv);
    printf("%s\n", args.to_string().c_str());

    srand(args.seed);

    common::Trainer* trainer = new transr::Trainer(args);
    trainer->loadFiles();
    trainer->train();
    trainer->write();
    delete(trainer);
}
