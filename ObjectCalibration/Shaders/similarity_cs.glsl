#version 460

layout(local_size_x = 4, local_size_y = 4) in;

layout(rgba8, binding = 0) uniform readonly image2D target;
layout(rgba8, binding = 1) uniform readonly image2D other;

layout(binding = 2, offset = 0) uniform atomic_uint truePositive;
layout(binding = 2, offset = 4) uniform atomic_uint falsePositive;
layout(binding = 2, offset = 8) uniform atomic_uint falseNegative;
layout(binding = 2, offset = 12) uniform atomic_uint diceNumerator;
layout(binding = 2, offset = 16) uniform atomic_uint diceDenominator;
layout(binding = 2, offset = 20) uniform atomic_uint meanAbsoluteError;

// The coefficient to apply when rounding float numbers
uniform float multiplication_before_round = 1.0;

// Compute the similarity between two textures
void main()
{
	// Resolution of the textures
	const ivec2 targetSize = imageSize(target);
	const ivec2 otherSize = imageSize(other);

	// Coordinates on the texture
	const ivec2 coords = ivec2(gl_GlobalInvocationID);

	if (all(lessThanEqual(coords, targetSize)) && all(lessThanEqual(coords, otherSize)))
	{
		const vec4 pixelTarget = imageLoad(target, coords);
		const vec4 pixelOther = imageLoad(other, coords);

		// Compute fuzzy Dice coefficient
		const float otherTransparency = pixelOther.a;
		const float targetTransparency = pixelTarget.a;

		const float product = otherTransparency * targetTransparency;
		const float sum = otherTransparency + targetTransparency;

		const int productInt = int(round(multiplication_before_round * product));
		const int sumInt = int(round(multiplication_before_round * sum));

		atomicCounterAdd(diceNumerator, productInt);
		atomicCounterAdd(diceDenominator, sumInt);

		// Compute truePositive, falsePositive, falseNegative
		const bool targetNotTransparent = targetTransparency > 0.0;
		const bool otherNotTransparent = otherTransparency > 0.0;
		
		if (otherNotTransparent && targetNotTransparent)
		{
			atomicCounterIncrement(truePositive);

			// Compute Mean Absolute Error in the overlap zone
			const vec4 diff = abs(pixelOther - pixelTarget);
			const float sumDiff = (diff.r + diff.g + diff.b + diff.a) / 4.0;
			const int sumDiffInt = int(round(multiplication_before_round * sumDiff));
			atomicCounterAdd(meanAbsoluteError, sumDiffInt);
		}
		else if (otherNotTransparent && !targetNotTransparent)
		{
			atomicCounterIncrement(falsePositive);
		}
		else if (!otherNotTransparent && targetNotTransparent)
		{
			atomicCounterIncrement(falseNegative);
		}
	}
}