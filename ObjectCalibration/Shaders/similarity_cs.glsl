#version 430

layout(local_size_x = 4, local_size_y = 4) in;

layout(r32f, binding = 0) uniform image2D target;
layout(r32f, binding = 1) uniform image2D other;


// Compute the similarity between two textures
void main()
{
	// Resolution of the textures
	const ivec2 targetSize = imageSize(target);
	const ivec2 otherSize = imageSize(other);

	// Coordinates on the texture
	const ivec2 coords = ivec2(gl_GlobalInvocationID);

	// If within the interior of the textures
	if (all(greaterThan(coords, ivec2(0, 0)))
	 && all(lessThan(coords, targetSize - ivec2(1, 1)))
	 && all(lessThan(coords, otherSize - ivec2(1, 1))))
	{
		const vec4 pixelTarget = imageLoad(target, coords);
		const vec4 pixelOther = imageLoad(other, coords);

		// TODO: Manage transparency
	}

	// TODO: Output the parrallel reduction in a buffer
}