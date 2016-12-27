#include <iostream>
#include <string>

#include "transh/transHTrainer.h"
#include "common/trainer.h"
#include "common/utils.h"

int main(int argc, char**argv) {
    srand((unsigned)time(NULL));

    common::TrainerArguments args = common::parseArgs(argc, argv);
    printf("%s\n", args.to_string());

    common::Trainer* trainer = new transh::TransHTrainer(args);
    trainer->loadFiles();
    trainer->train();
    trainer->write();
}
