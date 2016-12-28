#include "transh/trainer.h"

#include <cmath>

#include "common/constants.h"
#include "common/utils.h"

namespace transh {

void Trainer::gradientUpdate(int head, int tail, int relation, bool corrupted) {
    double beta = corrupted ? 1 : -1;

    double headSum = 0;
    double tailSum = 0;
    double sum_x = 0;

    for (int i = 0; i < embeddingSize_; i++) {
        headSum += weights_[relation][i] * entityVec_[head][i];
        tailSum += weights_[relation][i] * entityVec_[tail][i];
    }

    for (int i = 0; i < embeddingSize_; i++) {
        double x = 2 * (entityVec_[tail][i] - tailSum * weights_[relation][i] - (entityVec_[head][i] - headSum * weights_[relation][i]) - relationVec_[relation][i]);

        //for L1 distance function
        if (x > 0) {
            x = 1;
        } else {
            x = -1;
        }

        sum_x += x * weights_[relation][i];
        relationVec_next_[relation][i] -= beta * learningRate_ * x;

        entityVec_next_[head][i] -= beta * learningRate_ * x;
        entityVec_next_[tail][i] += beta * learningRate_ * x;

        weights_next_[relation][i] += beta * learningRate_ * x * headSum;
        weights_next_[relation][i] -= beta * learningRate_ * x * tailSum;
    }

    for (int i = 0; i < embeddingSize_; i++) {
        weights_next_[relation][i] += beta * learningRate_ * sum_x * entityVec_[head][i];
        weights_next_[relation][i] -= beta * learningRate_ * sum_x * entityVec_[tail][i];
    }

    common::norm(relationVec_next_[relation]);
    common::norm(entityVec_next_[head]);
    common::norm(entityVec_next_[tail]);

    common::norm(weights_next_[relation], false);
    // TODO(eriq): By normalizing against the weights here, are we missing a normalization with
    //  for the non-corrupted tupples against the updated weights? (updated during the corruption
    //  gradient update).
    common::norm(relationVec_next_[relation], weights_next_[relation], learningRate_);
    common::norm(entityVec_next_[head], weights_next_[relation], learningRate_);
    common::norm(entityVec_next_[tail], weights_next_[relation], learningRate_);
}

double Trainer::initialEmbeddingValue() {
    return common::randn(0, 1.0 / embeddingSize_, -1, 1);
}

void Trainer::postbatch() {
    relationVec_ = relationVec_next_;
    entityVec_ = entityVec_next_;
    weights_ = weights_next_;
}

void Trainer::prebatch() {
    relationVec_next_ = relationVec_;
    entityVec_next_ = entityVec_;
    weights_next_ = weights_;
}

void Trainer::prepTrain() {
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

double Trainer::tripleEnergy(int head, int tail, int relation) {
    double headSum = 0;
    double tailSum = 0;

    for (int i = 0; i < embeddingSize_; i++) {
        headSum += weights_[relation][i] * entityVec_[head][i];
        tailSum += weights_[relation][i] * entityVec_[tail][i];
    }

    double energy = 0;
    for (int i = 0; i < embeddingSize_; i++) {
        energy += std::fabs(entityVec_[tail][i] - tailSum * weights_[relation][i] - (entityVec_[head][i] - headSum * weights_[relation][i]) - relationVec_[relation][i]);
    }

    return energy;
}

void Trainer::write() {
    Trainer::write();

    FILE* weightOutFile = fopen((outputDir_ + "/" + WEIGHT_EMBEDDING_FILE_BASENAME + "." + methodName()).c_str(), "w");
    for (int i = 0; i < numRelations_; i++) {
        for (int j=0; j < embeddingSize_; j++) {
            fprintf(weightOutFile, "%.6lf\t", weights_[i][j]);
        }
        fprintf(weightOutFile, "\n");
    }
    fclose(weightOutFile);
}

}  // namespace transh
