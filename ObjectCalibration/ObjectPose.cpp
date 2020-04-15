#include "ObjectPose.h"

#include <QFileDialog>
#include <QTextStream>

const float ObjectPose::TranslationRange = 0.2f;
const float ObjectPose::RotationRange = 45.0f;

ObjectPose readPose(const QString& filename)
{
	ObjectPose pose;
	
	QFile file(filename);

	if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		float x, y, z, xAngle, yAngle, zAngle;
		QTextStream stream(&file);
		stream >> x >> y >> z >> xAngle >> yAngle >> zAngle;

		pose.translation.setX(x);
		pose.translation.setY(y);
		pose.translation.setZ(z);
		pose.rotation.setX(xAngle);
		pose.rotation.setY(yAngle);
		pose.rotation.setZ(zAngle);

		file.close();
	}

	return pose;
}

float maxTranslationError(const ObjectPose& a, const ObjectPose& b)
{
	const auto diffX = std::abs(a.translation.x() - b.translation.x());
	const auto diffY = std::abs(a.translation.x() - b.translation.x());
	const auto diffZ = std::abs(a.translation.x() - b.translation.x());

	return std::max({ diffX, diffY, diffZ });
}

float maxRotationError(const ObjectPose& a, const ObjectPose& b)
{
	const auto diffX = std::abs(a.rotation.x() - b.rotation.x());
	const auto diffY = std::abs(a.rotation.x() - b.rotation.x());
	const auto diffZ = std::abs(a.rotation.x() - b.rotation.x());
	
	return std::max({ diffX, diffY, diffZ });
}

float avgTranslationError(const ObjectPose& a, const ObjectPose& b)
{
	const auto diffX = std::abs(a.translation.x() - b.translation.x());
	const auto diffY = std::abs(a.translation.x() - b.translation.x());
	const auto diffZ = std::abs(a.translation.x() - b.translation.x());
	
	return (diffX + diffY + diffZ) / 3.0f;
}

float avgRotationError(const ObjectPose& a, const ObjectPose& b)
{
	const auto diffX = std::abs(a.rotation.x() - b.rotation.x());
	const auto diffY = std::abs(a.rotation.x() - b.rotation.x());
	const auto diffZ = std::abs(a.rotation.x() - b.rotation.x());

	return (diffX + diffY + diffZ) / 3.0f;
}
