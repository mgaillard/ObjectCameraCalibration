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
	const auto xAngle = parameters(0);
	const auto yAngle = parameters(1);
	const auto zAngle = parameters(2);

	const auto similarity = m_viewerWidget->renderAndComputeSimilarityCpu(xAngle, yAngle, zAngle);

	qDebug() << "similarity " << similarity
	         << " xAngle = " << xAngle
	         << " yAngle = " << yAngle
	         << " zAngle = " << zAngle;
	
	return similarity;
}

QVector3D runBundleAdjustment(
	ViewerWidget* viewerWidget,
	const QImage& targetImage,
	float xAngleInitial,
	float yAngleInitial,
	float zAngleInitial)
{
	viewerWidget->setTargetImage(targetImage);

	const BundleAdjustment problem(viewerWidget);
	
	// Initial configuration
	BundleAdjustment::ColumnVector parameters(3);
	parameters(0) = xAngleInitial;
	parameters(1) = yAngleInitial;
	parameters(2) = zAngleInitial;

	const float eps = 1e-1;
	find_max_using_approximate_derivatives(bfgs_search_strategy(),
		                                   objective_delta_stop_strategy(1e-6),
		                                   problem,
		                                   parameters,
		                                   1.0,
		                                   eps);

	const auto xAngle = parameters(0);
	const auto yAngle = parameters(1);
	const auto zAngle = parameters(2);

	qDebug() << xAngle;
	qDebug() << yAngle;
	qDebug() << zAngle;

	return QVector3D(xAngle, yAngle, zAngle);
}
