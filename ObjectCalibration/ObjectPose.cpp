#include "ObjectPose.h"

#include <QFileDialog>
#include <QQuaternion>
#include <QTextStream>

#include "MathUtils.h"

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

bool savePose(const ObjectPose& pose, const QString& filename)
{
	QFile file(filename);
	
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream stream(&file);

		stream.setRealNumberNotation(QTextStream::FixedNotation);
		stream.setRealNumberPrecision(2);
		
		stream << pose.translation.x() << "\n";
		stream << pose.translation.y() << "\n";
		stream << pose.translation.z() << "\n";
		stream << pose.rotation.x() << "\n";
		stream << pose.rotation.y() << "\n";
		stream << pose.rotation.z();
		
		file.close();
		return true;
	}
		
	return false;
}

float maxTranslationError(const ObjectPose& a, const ObjectPose& b)
{
	const auto diffX = std::abs(a.translation.x() - b.translation.x());
	const auto diffY = std::abs(a.translation.y() - b.translation.y());
	const auto diffZ = std::abs(a.translation.z() - b.translation.z());

	return std::max({ diffX, diffY, diffZ });
}

float maxRotationError(const ObjectPose& a, const ObjectPose& b)
{
	const auto diffX = std::abs(a.rotation.x() - b.rotation.x());
	const auto diffY = std::abs(a.rotation.y() - b.rotation.y());
	const auto diffZ = std::abs(a.rotation.z() - b.rotation.z());
	
	return std::max({ diffX, diffY, diffZ });
}

float translationError(const ObjectPose& a, const ObjectPose& b)
{
	return a.translation.distanceToPoint(b.translation);;
}

float rotationError(const ObjectPose& a, const ObjectPose& b)
{
	// Source
	// Huynh, D. Q. (2009). Metrics for 3D rotations: Comparison and analysis.
	// Journal of Mathematical Imaging and Vision, 35(2), 155-164.
	
	const auto qa = QQuaternion::fromEulerAngles(a.rotation).normalized();
	const auto qb = QQuaternion::fromEulerAngles(b.rotation).normalized();

	const auto dot = std::abs(QQuaternion::dotProduct(qa, qb));
	
	return std::acos(clamp(dot, 0.0f, 1.0f));
}
