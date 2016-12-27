#include <iostream>
#include <string>

#include "transh/transHTrainer.h"
#include "common/trainer.h"
#include "common/utils.h"

int main(int argc, char**argv) {
    srand((unsigned)time(NULL));

    int method = 1;
    int n = 100;
    double rate = 0.001;
    double margin = 1;
    int i;
    std::string version;

    if ((i = common::argpos((char *)"-size", argc, argv)) > 0) n = atoi(argv[i + 1]);
    if ((i = common::argpos((char *)"-margin", argc, argv)) > 0) margin = atoi(argv[i + 1]);
    if ((i = common::argpos((char *)"-rate", argc, argv)) > 0) rate = atof(argv[i + 1]);
    if ((i = common::argpos((char *)"-method", argc, argv)) > 0) method = atoi(argv[i + 1]);

    if (method) {
        version = "bern";
    } else {
        version = "unif";
    }

    std::cout << "size = " << n << std::endl;
    std::cout << "learing rate = " << rate << std::endl;
    std::cout << "margin = " << margin << std::endl;
    std::cout << "method = " << version << std::endl;

    common::Trainer* trainer = new transh::TransHTrainer("", "", n, rate, margin, method, -1, -1);
    trainer->loadFiles();
    trainer->train();
    trainer->write();
}
