#include "transh/transHTrainer.h"

#include <cmath>

#include "common/constants.h"
#include "common/utils.h"

namespace transh {

void TransHTrainer::gradientUpdate(int head, int tail, int relation, bool corrupted) {
    double beta = corrupted ? 1 : -1;

    double headSum = 0;
    double tailSum = 0;
    double sum_x = 0;

    for (int i = 0; i < embeddingSize_; i++) {
        headSum += weights_[relation][i] * entity_vec_[head][i];
        tailSum += weights_[relation][i] * entity_vec_[tail][i];
    }

    for (int i = 0; i < embeddingSize_; i++) {
        double x = 2 * (entity_vec_[tail][i] - tailSum * weights_[relation][i] - (entity_vec_[head][i] - headSum * weights_[relation][i]) - relation_vec_[relation][i]);

        //for L1 distance function
        if (x > 0) {
            x = 1;
        } else {
            x = -1;
        }

        sum_x += x * weights_[relation][i];
        relation_vec_next_[relation][i] -= beta * learningRate_ * x;

        entity_vec_next_[head][i] -= beta * learningRate_ * x;
        entity_vec_next_[tail][i] += beta * learningRate_ * x;

        weights_next_[relation][i] += beta * learningRate_ * x * headSum;
        weights_next_[relation][i] -= beta * learningRate_ * x * tailSum;
    }

    for (int i = 0; i < embeddingSize_; i++) {
        weights_next_[relation][i] += beta * learningRate_ * sum_x * entity_vec_[head][i];
        weights_next_[relation][i] -= beta * learningRate_ * sum_x * entity_vec_[tail][i];
    }

    common::norm(relation_vec_next_[relation]);
    common::norm(entity_vec_next_[head]);
    common::norm(entity_vec_next_[tail]);

    common::norm(weights_next_[relation], false);
    // TODO(eriq): By normalizing against the weights here, are we missing a normalization with
    //  for the non-corrupted tupples against the updated weights? (updated during the corruption
    //  gradient update).
    common::norm(relation_vec_next_[relation], weights_next_[relation], learningRate_);
    common::norm(entity_vec_next_[head], weights_next_[relation], learningRate_);
    common::norm(entity_vec_next_[tail], weights_next_[relation], learningRate_);
}

double TransHTrainer::initialEmbeddingValue() {
    return common::randn(0, 1.0 / embeddingSize_, -1, 1);
}

void TransHTrainer::postbatch() {
    relation_vec_ = relation_vec_next_;
    entity_vec_ = entity_vec_next_;
    weights_ = weights_next_;
}

void TransHTrainer::prebatch() {
    relation_vec_next_ = relation_vec_;
    entity_vec_next_ = entity_vec_;
    weights_next_ = weights_;
}

void TransHTrainer::prepTrain() {
    Trainer::prepTrain();

    weights_.resize(numRelations_);
    for (int i = 0; i < numRelations_; i++) {
        weights_[i].resize(embeddingSize_);
        for (int j = 0; j < embeddingSize_; j++) {
            weights_[i][j] = initialEmbeddingValue();
        }
        common::norm(weights_[i], false);
    }
}

double TransHTrainer::tripleEnergy(int head, int tail, int relation) {
    double headSum = 0;
    double tailSum = 0;

    for (int i = 0; i < embeddingSize_; i++) {
        headSum += weights_[relation][i] * entity_vec_[head][i];
        tailSum += weights_[relation][i] * entity_vec_[tail][i];
    }

    double energy = 0;
    for (int i = 0; i < embeddingSize_; i++) {
        energy += std::fabs(entity_vec_[tail][i] - tailSum * weights_[relation][i] - (entity_vec_[head][i] - headSum * weights_[relation][i]) - relation_vec_[relation][i]);
    }

    return energy;
}

void TransHTrainer::write() {
    Trainer::write();

    FILE* weightOutFile = fopen((outputDir_ + "/" + WEIGHT_OUT_FILE_BASENAME + "." + methodName()).c_str(), "w");
    for (int i = 0; i < numRelations_; i++) {
        for (int j=0; j < embeddingSize_; j++) {
            fprintf(weightOutFile, "%.6lf\t", weights_[i][j]);
        }
        fprintf(weightOutFile, "\n");
    }
    fclose(weightOutFile);
}

}  // namespace transh
