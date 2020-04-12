#include "BundleAdjustment.h"

#include <QtDebug>

#include <dlib/optimization.h>

using namespace dlib;

BundleAdjustment::BundleAdjustment(ViewerWidget* viewerWidget) :
	m_viewerWidget(viewerWidget)
{
	
}

double BundleAdjustment::operator()(const ColumnVector& parameters) const
{
	const auto pose = parametersToObjectPose(parameters);

	const auto similarity = m_viewerWidget->renderAndComputeSimilarityGpu(pose);

	qDebug() << "similarity " << similarity
	         << " translation = " << pose.translation
	         << " rotation = " << pose.rotation;
	
	return similarity;
}

BundleAdjustment::ColumnVector BundleAdjustment::objectPoseToParameters(const ObjectPose& pose)
{
	const auto translation = pose.normalizedTranslation();
	const auto rotation = pose.normalizedRotation();
	
	ColumnVector parameters(6);

	parameters(0) = translation.x();
	parameters(1) = translation.y();
	parameters(2) = translation.z();
	parameters(3) = rotation.x();
	parameters(4) = rotation.y();
	parameters(5) = rotation.z();
	
	return parameters;
}

ObjectPose BundleAdjustment::parametersToObjectPose(const ColumnVector& parameters)
{
	ObjectPose pose;

	pose.setNormalizedTranslation(QVector3D(
		parameters(0),
		parameters(1),
		parameters(2)
	));

	pose.setNormalizedRotation(QVector3D(
		parameters(3),
		parameters(4),
		parameters(5)
	));
	
	return pose;
}

ObjectPose runBundleAdjustment(
	ViewerWidget* viewerWidget,
	const QImage& targetImage,
	const ObjectPose& pose)
{
	viewerWidget->setTargetImage(targetImage);

	const BundleAdjustment problem(viewerWidget);
	
	// Initial configuration
	auto parameters = BundleAdjustment::objectPoseToParameters(pose);

	const float eps = 1e-1;
	find_max_using_approximate_derivatives(bfgs_search_strategy(),
		                                   objective_delta_stop_strategy(1e-6),
		                                   problem,
		                                   parameters,
		                                   1.0,
		                                   eps);

	return BundleAdjustment::parametersToObjectPose(parameters);
}
