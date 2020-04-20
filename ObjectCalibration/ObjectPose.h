#pragma once

#include <QVector3D>

struct ObjectPose
{
	/**
	 * \brief 3D translation from the origin to the center of the object
	 */
	QVector3D translation;

	/**
	 * \brief Euler angles (ZXY) in degrees
	 */
	QVector3D rotation;

	static const QVector3D TranslationRange;
	static const QVector3D RotationRange;

	QVector3D normalizedTranslation() const
	{
		return {
			translation.x() / TranslationRange.x(),
			translation.y() / TranslationRange.y(),
			translation.z() / TranslationRange.z()
		};
	}

	QVector3D normalizedRotation() const
	{
		return {
			rotation.x() / RotationRange.x(),
			rotation.y() / RotationRange.y(),
			rotation.z() / RotationRange.z()
		};
	}

	void setNormalizedTranslation(const QVector3D& normalizedTranslation)
	{
		translation.setX(normalizedTranslation.x() * TranslationRange.x());
		translation.setY(normalizedTranslation.y() * TranslationRange.y());
		translation.setZ(normalizedTranslation.z() * TranslationRange.z());
	}

	void setNormalizedRotation(const QVector3D& normalizedRotation)
	{
		rotation.setX(normalizedRotation.x() * RotationRange.x());
		rotation.setY(normalizedRotation.y() * RotationRange.y());
		rotation.setZ(normalizedRotation.z() * RotationRange.z());
	}
};

ObjectPose readPose(const QString& filename);

bool savePose(const ObjectPose& pose,const QString& filename);

float maxTranslationError(const ObjectPose& a, const ObjectPose& b);

float maxRotationError(const ObjectPose& a, const ObjectPose& b);

float translationError(const ObjectPose& a, const ObjectPose& b);

float rotationError(const ObjectPose& a, const ObjectPose& b);
