#pragma once
#include "model.h"
#include "Color.h"
#include "Camera.h"
#include "Shader.h"
#include <Eigen/Core>
using namespace std;
using namespace Eigen;
# define M_PI 3.14159265358979323846

class Renderer
{
public:
	enum class DrawMode {
		LINE,
		TRIANGLE
	};

	Renderer() = default;
	Renderer(int w, int h, unsigned int** fb, Camera *camera, Vector3f lightDir);
	~Renderer();
	void bufferClear(); // clear frame buffer and z buffer
	
	// set pixel with color c, 左下角为坐标原点
	void set(const int &x, const int &y, const Color &c) { 
		frameBuffer_[height - y][x] = c.hex; }
	void setZBuffer(const int &x, const int &y, const float &z) {
		zBuffer[height - y][x] = z; }
	void setCamera(Camera *camera) { camera_ = camera; }
	void drawLine(int x0, int y0, int x1, int y1, const Color& c);
	void drawLine(Vector2i t0, Vector2i t1, const Color& c);
	void drawTriangle(Vector2i t0, Vector2i t1, Vector2i t2, const Color &c);
	void drawTriangle(Vector3f t0, Vector3f t1, Vector3f t2, const Color &c);
	void drawTriangle(Vector3f screenCoords[3], FShader *shader, int iface, unsigned int **frameBuff, float **zBuffer);
	void drawModel(Model *model, DrawMode mode, Matrix4f modelMatrix = Matrix4f::Identity()); // draw obj model

private:
	int width;
	int height;
	unsigned int** frameBuffer_;
	float** zBuffer;
	Vector3f lightPos_;
	Vector3f lightDir_;
	Model *model;
	Camera *camera_;
	Matrix4f projectionMatrix_;
	Matrix4f viewPortMatrix_;

	float FOV_;
	float zNear_;
	float zFar_;

	Shader *shader;
	// Shadow
	DepthShader *depthShader;
	unsigned int **depthMap;
	float **shadowBuffer;

	inline bool isLegal(const int& x, const int& y) {
		if (x < 0 || x >= width || y < 0 || y >= height) return false;
		return true;
	}

	int isLeft(Vector2i l0, Vector2i l1, Vector2i p);
	bool isInTriangle(const Vector2i& v0, const Vector2i& v1, const Vector2i& v2, const Vector2i& p); // 判断点p是否在三角形内
	std::pair<float, float> barycentric(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector2i &p); // 计算p的重心坐标
	Vector3f transform(const Vector3f &p, const Matrix4f &transformMatrix); // 3d点p与4x4变换矩阵相乘
	void initProjectionMatrix();
	Matrix4f computeProjectionMatrix(float width, float height, float FOV, float zNear, float zFar);
	bool isInView(const Vector3f &p);
	bool isInWindow(const Vector3f &p);
};

