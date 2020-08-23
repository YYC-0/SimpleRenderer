#pragma once
#include <Eigen/Core>
using namespace Eigen;

class Camera
{
public:
	enum class Action {
		UP, DOWN, LEFT, RIGHT, ZOOM_IN, ZOOM_OUT
	};
	Camera(Vector3f pos = Vector3f(0, 0, 3), Vector3f target = Vector3f(0, 0, 0), 
		Vector3f up = Vector3f(0,1,0));

	inline Matrix4f getViewMatrix(){ return viewMatrix_; }
	inline Vector3f getPos() { return pos_; }
	inline Vector3f getDirection() { return direction_; }
	void updateLookAt(Vector3f pos, Vector3f target);
	void updateLookAt();
	void move(Action action);

private:
	Vector3f pos_; // camera position
	float distance; // camera target direction
	Vector3f target_; // camera target
	Vector3f direction_; // camera direction
	Vector3f cameraUp_; // up vector
	Vector3f cameraRight_; // right vector
	Vector3f up_;

	Matrix4f viewMatrix_;
};
