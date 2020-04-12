#pragma once

#include <dlib/matrix/matrix.h>

#include "ObjectPose.h"
#include "ViewerWidget.h"

class BundleAdjustment
{
public:
	/**
	 * \brief Column vector type used by dlib
	 */
	using ColumnVector = dlib::matrix<double, 0, 1>;

	BundleAdjustment(ViewerWidget* viewerWidget);

	/**
	 * \brief Compute the value of the objective function
	 * \return The value of the objective function
	 */
	double operator()(const ColumnVector& parameters) const;

	static ColumnVector objectPoseToParameters(const ObjectPose& pose);

	static ObjectPose parametersToObjectPose(const ColumnVector& parameters);
	
private:

	ViewerWidget* m_viewerWidget;
};

ObjectPose runBundleAdjustment(ViewerWidget* viewerWidget,
	                           const QImage& targetImage,
	                           const ObjectPose& pose);
