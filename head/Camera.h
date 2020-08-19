#pragma once
#include <Eigen/Core>
using namespace Eigen;

class Camera
{
public:
	Camera(Vector3f pos = Vector3f(0, 0, -5), Vector3f target = Vector3f(0, 0, 0), 
		Vector3f up = Vector3f(0,1,0));

	inline Matrix4f getViewMatrix(){ return viewMatrix_; }
	inline Vector3f getPos() { return pos_; }
	void updateLookAt(Vector3f pos, Vector3f target);

private:
	Vector3f pos_; // camera position
	Vector3f target_; // camera target
	Vector3f direction_; // camera direction
	Vector3f cameraUp_; // up vector
	Vector3f cameraRight_; // right vector
	Vector3f up_;

	Matrix4f viewMatrix_;
};
