#pragma once

#include <QImage>

float computeSimilarity(const QImage& image, const QImage& target);

QImage diceSimilarityErrorMap(const QImage& image, const QImage& target);

QImage mseSimilarityErrorMap(const QImage& image, const QImage& target);
