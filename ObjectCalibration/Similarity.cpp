#include "Similarity.h"

float computeSimilarity(const QImage& image, const QImage& target)
{
	assert(image.size() == target.size());

	double mae = 0.0;
	long long truePositives = 0;

	double diceNumerator = 0.0;
	double diceDenominator = 0.0;

	#pragma omp parallel for shared(mae, truePositives, diceNumerator, diceDenominator)
	for (int i = 0; i < image.height(); i++)
	{
		for (int j = 0; j < image.width(); j++)
		{
			const auto imageColor = image.pixelColor(j, i);
			const auto targetColor = target.pixelColor(j, i);

			const auto imageTransparency = imageColor.alphaF();
			const auto targetTransparency = targetColor.alphaF();

			#pragma omp atomic
			diceNumerator += imageTransparency * targetTransparency;
			#pragma omp atomic
			diceDenominator += imageTransparency + targetTransparency;
			
			// Present in the image and in the target
			if (imageTransparency > 0.0 && targetTransparency > 0.0)
			{
				#pragma omp atomic
				truePositives++;				
				#pragma omp atomic
				mae += std::abs(imageColor.valueF() - targetColor.valueF());
			}
		}
	}

	// mae is the mean absolute error only on the overlap zone
	mae /= float(truePositives);

	// Overlap between the two objects
	const auto diceCoefficient = 2.0f * (diceNumerator + 1.0) / (diceDenominator + 1.0);
	
	return 0.9 * diceCoefficient + 0.1 * mae;
}
