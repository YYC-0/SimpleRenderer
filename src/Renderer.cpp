#include "../head/Renderer.h"

#include <cmath>
#include <algorithm>
#include <vector>
#include <Eigen/Core>
#include <Eigen/Dense>
#include <iostream>

Renderer::Renderer(int w, int h, unsigned int** fb, Camera *camera, Vector3f lightDir)
{
	width = w;
	height = h;
	frameBuffer_ = fb;
	lightDir_ = lightDir;
	camera_ = camera;

	zNear_ = -0.1;
	zFar_ = -100.0;
	FOV_ = M_PI / 4.0;


	initProjectionMatrix();
	viewPortMatrix_ <<
		width/2,0,			0,		 width/2,
		0,		height/2,	0,		 height/2,
		0,		0,			1.0/2.0, 1.0/2.0,
		0,		0,			0,		 1;

	depthMap = new unsigned int *[height];
	shadowBuffer = new float *[height];
	zBuffer = new float *[height];
	for (int i = 0; i < height; ++i)
	{
		depthMap[i] = new unsigned int[width];
		shadowBuffer[i] = new float[width];
		zBuffer[i] = new float[width];
	}

	bufferClear();

	shader = new Shader(viewPortMatrix_, lightDir_, shadowBuffer, height, width);
	depthShader = new DepthShader(viewPortMatrix_);
}

Renderer::~Renderer()
{
	for (int i = 0; i < height; ++i)
	{
		delete[] depthMap[i];
		delete[] shadowBuffer[i];
		delete[] zBuffer[i];
	}
	delete shader;
	delete depthShader;
	delete zBuffer;
}

void Renderer::bufferClear()
{
	for (int i = 0; i < height; ++i)
		for (int j = 0; j < width; ++j)
		{
			frameBuffer_[i][j] = 0;
			zBuffer[i][j] = zFar_;

			depthMap[i][j] = 0;
			shadowBuffer[i][j] = zFar_;
		}
}


void Renderer::drawLine(int x0, int y0, int x1, int y1, const Color& c)
{
	bool steep = false;
	if (std::abs(x0 - x1) < std::abs(y0 - y1))
	{
		// if the line ls steep, transpose the image
		std::swap(x0, y0);
		std::swap(x1, y1);
		steep = true;
	}
	if (x0 > x1) {
		// make it left to right
		std::swap(x0, x1);
		std::swap(y0, y1);
	}
	int dx = x1 - x0;
	int dy = y1 - y0;
	int derror2 = abs(dy) * 2;
	int error2 = 0;
	int y = y0;
	for (int x = x0; x <= x1; x++) {
		if (steep)
			set(y, x, c); // if transposed, de-transpose
		else
			set(x, y, c);
		error2 += derror2;
		if (error2 > dx)
		{
			y += (y1 > y0 ? 1 : -1);
			error2 -= dx * 2;
		}
	}
}

void Renderer::drawLine(Vector2i t0, Vector2i t1, const Color& c)
{
	drawLine(t0.x(), t0.y(), t1.x(), t1.y(), c);
}

// 由屏幕二维坐标绘制三角形
void Renderer::drawTriangle(Vector2i t0, Vector2i t1, Vector2i t2, const Color& color)
{
	int max_x = max(max(t0.x(), t1.x()), t2.x());
	int min_x = min(min(t0.x(), t1.x()), t2.x());
	int max_y = max(max(t0.y(), t1.y()), t2.y());
	int min_y = min(min(t0.y(), t1.y()), t2.y());
	for(int y = min_y; y<=max_y; ++y)
		for (int x = min_x; x <= max_x; ++x)
		{
			Vector2i p(x, y);
			if (isInTriangle(t0, t1, t2, p))
			{
				set(x, y, color);
			}
		}
}

// 由屏幕三维坐标绘制三角形
void Renderer::drawTriangle(Vector3f t0, Vector3f t1, Vector3f t2, const Color &color)
{
	int max_x = max(max(t0.x(), t1.x()), t2.x());
	int min_x = min(min(t0.x(), t1.x()), t2.x());
	int max_y = max(max(t0.y(), t1.y()), t2.y());
	int min_y = min(min(t0.y(), t1.y()), t2.y());
	for (int y = min_y; y <= max_y; ++y)
		for (int x = min_x; x <= max_x; ++x)
		{
			Vector2i p(x, y);
			pair<float, float> uv = barycentric(t0, t1, t2, p);
			float u = uv.first, v = uv.second;
			if (u >= 0 && v >= 0 && u + v <= 1)
			{
				Vector3f AB = t1 - t0,
					AC = t2 - t0;
				Vector3f p3f = t0 + u * AB + v * AC;
				float z = p3f.z();
				if (isLegal(x, height - y))
					if (zBuffer[height - y][x] < z)
					{
						zBuffer[height - y][x] = z;
						set(x, y, color);
					}
			}
		}
}

void Renderer::drawTriangle(Vector3f screenCoords[3], FShader *shader, int iface, unsigned int **frameBuffer, float **zBuffer)
{
	int max_x = min((int)max(max(screenCoords[0].x(), screenCoords[1].x()), screenCoords[2].x()), width - 1);
	int min_x = max((int)min(min(screenCoords[0].x(), screenCoords[1].x()), screenCoords[2].x()), 0);
	int max_y = min((int)max(max(screenCoords[0].y(), screenCoords[1].y()), screenCoords[2].y()), height - 1);
	int min_y = max((int)min(min(screenCoords[0].y(), screenCoords[1].y()), screenCoords[2].y()), 0);

	for (int y = min_y; y <= max_y; ++y)
	{
		for (int x = min_x; x <= max_x; ++x)
		{
			Vector2i p(x, y);
			pair<float, float> uv = barycentric(screenCoords[0], screenCoords[1], screenCoords[2], p);
			float u = uv.first, v = uv.second;
			if (u >= 0 && v >= 0 && u + v <= 1)
			{
				Vector3f AB = screenCoords[1] - screenCoords[0],
					AC = screenCoords[2] - screenCoords[0];
				Vector3f p3f = screenCoords[0] + v * AB + u * AC; // u v ？？

				float z = p3f.z();
				z = z * z;
				if (isLegal(x, height - y) && z > zBuffer[height - y][x])
				{
					Color color = shader->fragment(iface, uv);
					zBuffer[height - y][x] = z;
					frameBuffer[height - y][x] = color.hex;
				}
			}
		}
	}
}

void Renderer::drawModel(Model *model, DrawMode mode, Matrix4f modelMatrix)
{
	this->model = model;
	Matrix4f cameraProjectionMatrix = computeProjectionMatrix(width, height, M_PI / 2, zNear_, zFar_);
	Camera lightCamera(lightDir_);

	shader->setModel(model);
	shader->setCameraPos(camera_->getPos());
	Matrix4f cameraT = viewPortMatrix_ * cameraProjectionMatrix * lightCamera.getViewMatrix() * modelMatrix;
	shader->setMatrix(modelMatrix, projectionMatrix_, camera_->getViewMatrix(), cameraT);

	depthShader->setModel(model);
	depthShader->setMatrix(modelMatrix, cameraProjectionMatrix, lightCamera.getViewMatrix());

	// render shadow map
#pragma omp parallel for num_threads(8)
	for (int i = 0; i < model->nfaces(); ++i)
	{
		Vector3f screenCoords[3];
		std::vector<int> face = model->face(i);
		for (int j = 0; j < 3; ++j)
		{
			screenCoords[j] = depthShader->vertex(i, j);
		}
		// 丢弃不在视口中的图元
		if (!isInWindow(screenCoords[0]) && 
			!isInWindow(screenCoords[1]) &&
			!isInWindow(screenCoords[2]))
			continue;
		drawTriangle(screenCoords, depthShader, i, depthMap, shadowBuffer);
	}

#pragma omp parallel for num_threads(8)
	for (int i = 0; i < model->nfaces(); ++i)
	{
		Vector3f screenCoords[3];
		std::vector<int> face = model->face(i);
		for (int j = 0; j < 3; ++j)
		{
			screenCoords[j] = shader->vertex(i, j);
		}
		// 丢弃不在视口中的图元
		if (!isInWindow(screenCoords[0]) &&
			!isInWindow(screenCoords[1]) &&
			!isInWindow(screenCoords[2]))
			continue;
		shader->computeTBN(i);
		drawTriangle(screenCoords, shader, i, frameBuffer_, zBuffer);
	}
}

// return >0 if p left of line l0-l1
// ==0 if p on the line
// <0 if p right
int Renderer::isLeft(Vector2i l0, Vector2i l1, Vector2i p)
{
	return (l1.x() - l0.x()) * (p.y() - l0.y())
		- (p.x() - l0.x()) * (l1.y() - l0.y());
}

// 判断点p是否在三角形内
// 重心坐标法
bool Renderer::isInTriangle(const Vector2i& v0, const Vector2i& v1, const Vector2i& v2, const Vector2i& p)
{
	Vector2i ab = v1 - v0,
		ac = v2 - v0,
		ap = p - v0;
	long long abab = ab.dot(ab),
		abac = ab.dot(ac),
		acab = ac.dot(ab),
		acac = ac.dot(ac),
		apac = ap.dot(ac),
		apab = ap.dot(ab);
	long long denominator = acac * abab - acab * abac;
	if (denominator == 0)
		return false;
	double u = (double)(abab * apac - acab * apab) / (double)denominator;
	double v = (double)(acac * apab - acab * apac) / (double)denominator;
	if (u >= 0 && v >= 0 && u + v <= 1)
		return true;
	return false;
}

// 输入三角形v0v1v2和二维点p (均为屏幕坐标系）
// 输出u v
pair<float, float> Renderer::barycentric(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector2i &p)
{
	Vector2f v_0(v0.x(), v0.y()),
		v_1(v1.x(), v1.y()),
		v_2(v2.x(), v2.y()),
		pp(p.x(), p.y());

	Vector2f ab = v_1 - v_0,
		ac = v_2 - v_0,
		ap = pp - v_0;
	double abab = ab.dot(ab),
		abac = ab.dot(ac),
		acab = ac.dot(ab),
		acac = ac.dot(ac),
		apac = ap.dot(ac),
		apab = ap.dot(ab);
	double denominator = acac * abab - acab * abac;
	double u = (float)(abab * apac - acab * apab) / (float)denominator;
	double v = (float)(acac * apab - acab * apac) / (float)denominator;

	return pair<float, float>(u, v);
}

Vector3f Renderer::transform(const Vector3f &p, const Matrix4f &transformMatrix)
{
	Vector4f p_h(p.x(), p.y(), p.z(), 1);
	Vector4f p_after = transformMatrix * p_h;
	return Vector3f(p_after.x() / p_after[3], p_after.y() / p_after[3], p_after.z() / p_after[3]);
}


// 初始化透视矩阵
void Renderer::initProjectionMatrix()
{
	float ar = (float)width / (float)height;
	float zRange = zFar_ - zNear_;
	float tanHalfFOV = tanf(FOV_ / 2.0);
	projectionMatrix_ <<
		1.0/(ar * tanHalfFOV), 0,	0, 0,
		0, 1.0/tanHalfFOV,			0, 0,
		0, 0, (-zNear_-zFar_)/zRange, -2.0*zFar_*zNear_/zRange,
		0, 0, -1.0, 0;
}

Matrix4f Renderer::computeProjectionMatrix(float width, float height, float FOV, float zNear, float zFar)
{
	float ar = width / height;
	float zRange = zFar - zNear;
	float tanHalfFOV = tanf(FOV / 2.0);
	Matrix4f projectionMatrix;
	projectionMatrix <<
		1.0 / (ar * tanHalfFOV), 0, 0, 0,
		0, 1.0 / tanHalfFOV, 0, 0,
		0, 0, (-zNear - zFar) / zRange, -2.0 * zFar * zNear / zRange,
		0, 0, -1.0, 0;
	return projectionMatrix;
}

// 检查p的坐标是否在裁剪空间中 [-1,1]
bool Renderer::isInView(const Vector3f &p)
{
	if (p.x() >= -1 && p.x() <= 1 &&
		p.y() >= -1 && p.y() <= 1)
		return true;
	return false;
}

bool Renderer::isInWindow(const Vector3f &p)
{
	if (p.x() >= 0 && p.x() <= width &&
		p.y() >= 0 && p.y() <= height)
		return true;
	return false;
}
