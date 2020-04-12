#pragma once

#include <QVector3D>

struct ObjectPose
{
	QVector3D translation;
	QVector3D rotation;

	static const float TranslationRange;
	static const float RotationRange;

	QVector3D normalizedTranslation() const
	{
		return {
			translation.x() / TranslationRange,
			translation.y() / TranslationRange,
			translation.z() / TranslationRange
		};
	}

	QVector3D normalizedRotation() const
	{
		return {
			rotation.x() / RotationRange,
			rotation.y() / RotationRange,
			rotation.z() / RotationRange
		};
	}

	void setNormalizedTranslation(const QVector3D& normalizedTranslation)
	{
		translation.setX(normalizedTranslation.x() * TranslationRange);
		translation.setY(normalizedTranslation.y() * TranslationRange);
		translation.setZ(normalizedTranslation.z() * TranslationRange);
	}

	void setNormalizedRotation(const QVector3D& normalizedRotation)
	{
		rotation.setX(normalizedRotation.x() * RotationRange);
		rotation.setY(normalizedRotation.y() * RotationRange);
		rotation.setZ(normalizedRotation.z() * RotationRange);
	}
};
