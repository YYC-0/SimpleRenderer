#include "../head/Camera.h"
#include <Eigen/Geometry>

//Camera::Camera()
//{
//	up_ = Vector3f(0, 0, 1);
//	updateLookAt(Vector3f(0, 0, 5), Vector3f(0, 0, 0));
//}

Camera::Camera(Vector3f pos, Vector3f target, Vector3f up): up_(up)
{
	updateLookAt(pos, target);
}

void Camera::updateLookAt(Vector3f pos, Vector3f target)
{
	pos_ = pos;
	target_ = target;
	direction_ = target_ - pos_;
	direction_.normalize();
	cameraRight_ = up_.cross(direction_);
	cameraRight_.normalize();
	cameraUp_ = direction_.cross(cameraRight_);
	cameraUp_.normalize();
	Matrix4f Minv = Matrix4f::Identity();
	Matrix4f Tr = Matrix4f::Identity();
	Minv.block<1, 3>(0, 0) = cameraRight_;
	Minv.block<1, 3>(1, 0) = cameraUp_;
	Minv.block<1, 3>(2, 0) = direction_;
	Tr.block<3, 1>(0, 3) = -pos_;

	viewMatrix_ = Minv * Tr;
}
