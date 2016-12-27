#include <iostream>
#include <string>

#include "transe/transETrainer.h"
#include "common/trainer.h"
#include "common/utils.h"

int main(int argc, char**argv) {
    srand((unsigned)time(NULL));

    common::TrainerArguments args = common::parseArgs(argc, argv);
    printf("%s\n", args.to_string());

    common::Trainer* trainer = new transe::TransETrainer(args);
    trainer->loadFiles();
    trainer->train();
    trainer->write();
}
