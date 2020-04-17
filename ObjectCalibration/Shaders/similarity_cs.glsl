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

// All components are in the range [0…1], including hue.
// Source: https://stackoverflow.com/questions/15095909/from-rgb-to-hsv-in-opengl-glsl
vec3 rgb2hsv(vec3 c)
{
	vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
	vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

	float d = q.x - min(q.w, q.y);
	float e = 1.0e-10;
	return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

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
			const vec3 pixelOtherHsv = rgb2hsv(vec3(pixelOther));
			const vec3 pixelTargetHsv = rgb2hsv(vec3(pixelTarget));

			const float pixelOtherValue = (pixelOtherHsv.z > 0.5) ? 1.0 : 0.0;
			const float pixelTargetValue = (pixelTargetHsv.z > 0.5) ? 1.0 : 0.0;

			if (pixelOtherValue == pixelTargetValue)
			{
				atomicCounterIncrement(meanAbsoluteError);
			}
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