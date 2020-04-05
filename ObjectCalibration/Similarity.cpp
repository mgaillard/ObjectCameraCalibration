#include "Similarity.h"

float computeSimilarity(const QImage& image, const QImage& target)
{
	assert(image.size() == target.size());

	long long truePositives = 0;
	long long falsePositives = 0;
	long long falseNegatives = 0;

	#pragma omp parallel for shared(truePositives, falsePositives, falseNegatives)
	for (int i = 0; i < image.height(); i++)
	{
		for (int j = 0; j < image.width(); j++)
		{
			const auto imageColor = image.pixelColor(j, i);
			const auto targetColor = target.pixelColor(j, i);

			const auto imageTransparency = imageColor.alpha() > 0;
			const auto targetTransparency = targetColor.alpha() > 0;

			// Present in the image and in the target
			if (imageTransparency && targetTransparency)
			{
				#pragma omp atomic
				truePositives++;
			}
			// Present in the image but not in the target
			else if (imageTransparency && !targetTransparency)
			{
				#pragma omp atomic
				falsePositives++;
			}
			// Not present in the image but present in the target
			else if (!imageTransparency && targetTransparency)
			{
				#pragma omp atomic
				falseNegatives++;
			}
		}
	}

	const auto diceCoefficient = 2.0f * float(truePositives) /
		(2.0f * float(truePositives) + float(falsePositives) + float(falseNegatives));

	return diceCoefficient;
}
