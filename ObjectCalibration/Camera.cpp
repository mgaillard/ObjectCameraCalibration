#include "camera.h"

#include <QPainter>

Camera::Camera(const QVector3D& eye,
	const QVector3D& at,
	const QVector3D& up,
	float fovy,
	float aspectRatio,
	float nearPlane,
	float farPlane) :
	m_eye(eye),
	m_at(at),
	m_up(up),
	m_fovy(fovy),
	m_aspectRatio(aspectRatio),
	m_near(nearPlane),
	m_far(farPlane)
{
	updateMatrices();
}

const QMatrix4x4& Camera::viewMatrix() const
{
	return m_viewMatrix;
}

const QMatrix4x4& Camera::projectionMatrix() const
{
	return m_projectionMatrix;
}

const QVector3D& Camera::eye() const
{
	return m_eye;
}

const QVector3D& Camera::at() const
{
	return m_at;
}

const QVector3D& Camera::up() const
{
	return m_up;
}

QVector3D Camera::right() const
{
	const QVector3D eyeToAt(m_at - m_eye);
	return QVector3D::crossProduct(eyeToAt, m_up).normalized();
}

float Camera::fovy() const
{
	return m_fovy;
}

float Camera::aspectRatio() const
{
	return m_aspectRatio;
}

float Camera::nearPlane() const
{
	return m_near;
}

float Camera::farPlane() const
{
	return m_far;
}

void Camera::setEye(const QVector3D& eye)
{
	m_eye = eye;

	updateMatrices();
}

void Camera::setAt(const QVector3D& at)
{
	m_at = at;

	updateMatrices();
}

void Camera::setUp(const QVector3D& up)
{
	m_up = up;

	updateMatrices();
}

void Camera::setFovy(float fovy)
{
	m_fovy = fovy;

	updateMatrices();
}

void Camera::setAspectRatio(float aspectRatio)
{
	m_aspectRatio = aspectRatio;

	updateMatrices();
}

void Camera::setNearPlane(float nearPlane)
{
	m_near = nearPlane;

	updateMatrices();
}

void Camera::setFarPlane(float farPlane)
{
	m_far = farPlane;

	updateMatrices();
}

void Camera::roundLeftRight(float angle)
{
	// Rotation around z
	QMatrix4x4 rotationMatrix;
	rotationMatrix.rotate(angle, 0.0, 0.0, 1.0);

	const QVector3D atToEye(m_eye - m_at);
	const QVector3D rotatedAtToEye = rotationMatrix * atToEye;
	m_eye = m_at + rotatedAtToEye;

	m_up = (rotationMatrix * m_up).normalized();

	updateMatrices();
}

void Camera::roundUpDown(float angle)
{
	const QVector3D atToEye(m_eye - m_at);
	const QVector3D rotateAxis = QVector3D::crossProduct(m_up, atToEye);

	QMatrix4x4 rotationMatrix;
	rotationMatrix.rotate(angle, rotateAxis);

	const QVector3D rotatedAtToEye = rotationMatrix * atToEye;
	m_eye = m_at + rotatedAtToEye;
	m_up = QVector3D::crossProduct(m_eye, rotateAxis).normalized();

	updateMatrices();
}

void Camera::moveLeftRight(float distance)
{
	const QVector3D eyeToAt(m_at - m_eye);
	const QVector3D right = QVector3D::crossProduct(eyeToAt, m_up).normalized();
	m_at += distance * right;
	m_eye += distance * right;

	updateMatrices();
}

void Camera::moveUpDown(float distance)
{
	const QVector3D up = m_up.normalized();
	m_at += distance * up;
	m_eye += distance * up;

	updateMatrices();
}

void Camera::moveForth(float distance)
{
	QVector3D eyeToAt(m_at - m_eye);
	eyeToAt.normalize();
	m_eye += distance * eyeToAt;

	updateMatrices();
}

void Camera::updateMatrices()
{
	m_viewMatrix.setToIdentity();
	m_viewMatrix.lookAt(m_eye, m_at, m_up);

	m_projectionMatrix.setToIdentity();
	m_projectionMatrix.perspective(m_fovy, m_aspectRatio, m_near, m_far);
}

OrbitCamera::OrbitCamera(const Camera& camera) :
	Camera(camera),
	m_roundMotionSensitivity(0.5f),
	m_moveMotionSensitivity(0.02f),
	m_mouseLeftButtonHold(false),
	m_mouseRightButtonHold(false),
	m_x0(0),
	m_y0(0)
{

}

OrbitCamera::OrbitCamera(const QVector3D& eye,
	const QVector3D& at,
	const QVector3D& up,
	float fovy,
	float aspectRatio,
	float nearPlane,
	float farPlane) :
	Camera(eye, at, up, fovy, aspectRatio, nearPlane, farPlane),
	m_roundMotionSensitivity(0.5f),
	m_moveMotionSensitivity(0.02f),
	m_mouseLeftButtonHold(false),
	m_mouseRightButtonHold(false),
	m_x0(0),
	m_y0(0)
{

}

void OrbitCamera::mouseLeftButtonPressed(int x, int y)
{
	m_mouseLeftButtonHold = true;
	m_mouseRightButtonHold = false;
	m_x0 = x;
	m_y0 = y;
}

void OrbitCamera::mouseRightButtonPressed(int x, int y)
{
	m_mouseLeftButtonHold = false;
	m_mouseRightButtonHold = true;
	m_x0 = x;
	m_y0 = y;
}

void OrbitCamera::mouseMoved(int x, int y)
{
	if (m_mouseLeftButtonHold)
	{
		roundLeftRight((m_x0 - x) * m_roundMotionSensitivity);
		roundUpDown((m_y0 - y) * m_roundMotionSensitivity);
	}

	if (m_mouseRightButtonHold)
	{
		moveLeftRight((m_x0 - x) * m_moveMotionSensitivity);
		moveUpDown((y - m_y0) * m_moveMotionSensitivity);
	}

	m_x0 = x;
	m_y0 = y;
}

void OrbitCamera::mouseReleased()
{
	m_mouseLeftButtonHold = false;
	m_mouseRightButtonHold = false;
}

void OrbitCamera::zoom(float distance)
{
	moveForth(distance);
}
