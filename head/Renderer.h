#pragma once
#include "model.h"
#include "Color.h"
#include "Camera.h"
#include <Eigen/Core>
using namespace std;
using namespace Eigen;

class Renderer
{
public:
	enum class DrawMode {
		LINE,
		TRIANGLE
	};

	Renderer() = default;
	Renderer(int w, int h, unsigned int** fb, float** zbuf, Camera *camera);

	void set(const int& x, const int& y, const Color& c); // set pixel with color c
	void drawLine(int x0, int y0, int x1, int y1, const Color& c);
	void drawLine(Vector2i t0, Vector2i t1, const Color& c);
	void drawTriangle(Vector2i t0, Vector2i t1, Vector2i t2, const Color& c);
	void drawTriangle(Vector3f t0, Vector3f t1, Vector3f t2, const Color &c);
	void drawTriangle(Vector3f t0, Vector3f t1, Vector3f t2,
						Vector2i uv0, Vector2i uv1, Vector2i uv2, float intensity);
	void drawTriangle(Vector3f t0, Vector3f t1, Vector3f t2,
				Vector2i uv0, Vector2i uv1, Vector2i uv2, 
				float ity0, float ity1, float ity2);
	void drawModel(Model *model, DrawMode mode, Matrix4f modelMatrix = Matrix4f::Identity());

private:
	int width;
	int height;
	unsigned int** frameBuffer;
	float** zBuffer;
	Vector3f light_dir;
	Model *model;
	Camera *camera_;
	Matrix4f projectMatrix_;

	inline bool isLegal(const int& x, const int& y) {
		if (x < 0 || x >= width || y < 0 || y >= height) return false;
		return true;
	}

	// return >0 if p left of line l0-l1
	// ==0 if p on the line
	// <0 if p right
	int isLeft(Vector2i l0, Vector2i l1, Vector2i p);
	bool isInTriangle(const Vector2i& v0, const Vector2i& v1, const Vector2i& v2, const Vector2i& p); // 判断点p是否在三角形内
	std::pair<float, float> barycentric(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector2i &p);
	Vector3f worldToScreen(const Vector3f &v) {
		return Vector3f(int((v.x() + 1.0) * width / 2.0 + 0.5), int((v.y() + 1.0) * height / 2.0 + 0.5), v.z());
	}
	Vector3f modelTransform(const Vector3f &p, const Matrix4f& transformMatrix); // 齐次矩阵
	Vector3f modelTransform(const Vector3f &p, const Matrix3f &rotation, const Vector3f &translate);
	Vector3f transform(const Vector3f &p, const Matrix4f &transformMatrix);
};

