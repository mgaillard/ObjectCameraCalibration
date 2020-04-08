#version 430

layout(local_size_x = 4, local_size_y = 4) in;

layout(rgba8, binding = 0) uniform readonly image2D target;
layout(rgba8, binding = 1) uniform readonly image2D other;

layout(binding = 2, offset = 0) uniform atomic_uint truePositive;
layout(binding = 2, offset = 4) uniform atomic_uint falsePositive;
layout(binding = 2, offset = 8) uniform atomic_uint falseNegative;

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

		const bool targetNotTransparent = pixelTarget.a > 0.0;
		const bool otherNotTransparent = pixelOther.a > 0.0;
		
		if (otherNotTransparent && targetNotTransparent)
		{
			atomicCounterIncrement(truePositive);
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