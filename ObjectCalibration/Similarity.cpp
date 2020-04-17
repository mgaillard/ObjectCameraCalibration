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
				mae += 1.0 - std::abs(imageColor.valueF() - targetColor.valueF());
			}
		}
	}

	// mae is the mean absolute error only on the overlap zone
	mae /= float(truePositives);

	// Overlap between the two objects
	const auto diceCoefficient = (2.0f * diceNumerator + 1.0) / (diceDenominator + 1.0);
	
	return 0.9 * diceCoefficient + 0.1 * mae;
}

QImage diceSimilarityErrorMap(const QImage& image, const QImage& target)
{
	assert(image.size() == target.size());

	QImage errorImage(image.size(), image.format());

	for (int i = 0; i < image.height(); i++)
	{
		for (int j = 0; j < image.width(); j++)
		{
			const auto imageColor = image.pixelColor(j, i);
			const auto targetColor = target.pixelColor(j, i);

			const auto imageTransparency = imageColor.alphaF();
			const auto targetTransparency = targetColor.alphaF();

			const auto diceNumerator = imageTransparency * targetTransparency;
			const auto diceDenominator = imageTransparency + targetTransparency;

			const auto dice = (2.0f * diceNumerator + 1.0) / (diceDenominator + 1.0);

			const QColor errorColor(
				int(255.0f * 2.0f * (dice - 0.5f)),
				int(255.0f * 2.0f * (dice - 0.5f)),
				int(255.0f * 2.0f * (dice - 0.5f)),
				255
			);

			errorImage.setPixelColor(j, i, errorColor);
		}
	}

	return errorImage;
}

QImage mseSimilarityErrorMap(const QImage& image, const QImage& target)
{
	assert(image.size() == target.size());

	QImage errorImage(image.size(), image.format());

	for (int i = 0; i < image.height(); i++)
	{
		for (int j = 0; j < image.width(); j++)
		{
			const auto imageColor = image.pixelColor(j, i);
			const auto targetColor = target.pixelColor(j, i);

			const auto imageTransparency = imageColor.alphaF();
			const auto targetTransparency = targetColor.alphaF();
			
			QColor errorColor(Qt::transparent);
			
			// Present in the image and in the target
			if (imageTransparency > 0.0 && targetTransparency > 0.0)
			{
				const auto imageValue = (imageColor.valueF() > 0.5) ? 1.0f : 0.0f;
				const auto targetValue = (targetColor.valueF() > 0.5) ? 1.0f : 0.0f;
				
				const auto gray = 1.0f - std::abs(imageValue - targetValue);

				errorColor.setRedF(gray);
				errorColor.setGreenF(gray);
				errorColor.setBlueF(gray);
				errorColor.setAlphaF(1.0f);
			}
			
			errorImage.setPixelColor(j, i, errorColor);
		}
	}

	return errorImage; 
}
