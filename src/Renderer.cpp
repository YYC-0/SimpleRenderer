#include "../head/Renderer.h"

#include <cmath>
#include <algorithm>
#include <vector>
#include <Eigen/Core>
#include <Eigen/Dense>
#include <iostream>
# define M_PI 3.14159265358979323846

Renderer::Renderer(int w, int h, unsigned int** fb, float **zbuf, Camera *camera)
{
	width = w;
	height = h;
	frameBuffer = fb;
	zBuffer = zbuf;
	light_dir = Vector3f(0, 0, 1);

	for(int i=0; i<height; ++i)
		for (int j = 0; j < width; ++j)
		{
			frameBuffer[i][j] = 0;
			zBuffer[i][j] = -10;
		}
	camera_ = camera;
	projectMatrix_ = Matrix4f::Identity();
	float c = camera_->getPos().norm();
	projectMatrix_(3, 2) = -1.0 / c;
}

// 左下角为坐标原点
void Renderer::set(const int& x, const int& y, const Color& c)
{
	if (isLegal(x, height - y))
		frameBuffer[height - y][x] = c.hex;
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
	//drawLine(t0, t1, c);
	//drawLine(t1, t2, c);
	//drawLine(t2, t0, c);

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

// draw triangle with texture
// Flat shading
// Input:	3 vertex(Vector3f)
//			3 uv coordnate(Vector2i)
//			light intensity
void Renderer::drawTriangle(Vector3f t0, Vector3f t1, Vector3f t2, Vector2i uv0, Vector2i uv1, Vector2i uv2, float intensity)
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
				Vector3f p3f = t0 + v * AB + u * AC; // u v ？？
				Vector2i uvAB = uv1 - uv0,
					uvAC = uv2 - uv0;
				Vector2f uvP = uv0.cast<float>() + v * uvAB.cast<float>() + u * uvAC.cast<float>(); // u v ？？
				Color color = model->diffuse(Vector2i(uvP.x(), uvP.y())) * intensity;
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

// draw triangle with texture
// Gouraud shading
void Renderer::drawTriangle(Vector3f t0, Vector3f t1, Vector3f t2, 
							Vector2i uv0, Vector2i uv1, Vector2i uv2, 
							float ity0, float ity1, float ity2)
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
				Vector3f p3f = t0 + v * AB + u * AC; // u v ？？
				Vector2i uvAB = uv1 - uv0,
					uvAC = uv2 - uv0;
				Vector2f uvP = uv0.cast<float>() + v * uvAB.cast<float>() + u * uvAC.cast<float>(); // u v ？？
				float iAB = ity1 - ity0,
					iAC = ity2 - ity0;
				float intensity = ity0 + v * iAB + u * iAC;
				if (intensity < 0)
					intensity = 0;
				Color color = model->diffuse(Vector2i(uvP.x(), uvP.y())) * intensity;
				if (color.r > 255)
					cout << "a";
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

void Renderer::drawModel(Model *model, DrawMode mode, Matrix4f modelMatrix)
{
	this->model = model;
	Matrix4f viewMatrix = camera_->getViewMatrix();
	Matrix4f MVP = projectMatrix_ * viewMatrix * modelMatrix;
	for (int i = 0; i < model->nfaces(); ++i)
	{
		std::vector<int> face = model->face(i);
		if (mode == DrawMode::LINE)
		{
			for (int j = 0; j < 3; ++j)
			{
				Vector3f v0 = model->vert(face[j]);
				Vector3f v1 = model->vert(face[(j + 1) % 3]);
				int x0 = (v0.x() + 1.0) * width / 2; // obj 中是-1到1
				int y0 = (v0.y() + 1.0) * height / 2.;
				int x1 = (v1.x() + 1.0) * width / 2.;
				int y1 = (v1.y() + 1.0) * height / 2.;
				this->drawLine(x0, y0, x1, y1, Color(255,255,255));
			}
		}
		else if (mode == DrawMode::TRIANGLE)
		{
			Vector3f screen_coords[3];
			Vector3f world_coords[3];
			Vector3f model_coords[3];
			Vector2i uv[3];
			float intensity[3];
			for (int j = 0; j < 3; ++j)
			{
				Vector3f v = model->vert(face[j]);
				world_coords[j] = v;
				model_coords[j] = transform(v, MVP);
				//Matrix3f modelMatrix = AngleAxisf(M_PI / 2, Vector3f(0, 1, 0)).matrix();
				//Vector3f translate(0, 0, 0);
				//model_coords[j] = modelTransform(v, modelMatrix, translate);
				screen_coords[j] = worldToScreen(model_coords[j]);
				intensity[j] = model->norm(i, j).dot(light_dir);
				uv[j] = model->uv(i, j);
			}

			//Vector3f n = (world_coords[1] - world_coords[0]).cross(world_coords[2] - world_coords[1]); // 三角形法线
			//n.normalize();
			//float intensity = n.dot(light_dir);
			//if (intensity > 0)
			//{
			//	// 无纹理
			//	//Color c(rand()%255, rand() % 255, rand() % 255);
			//	//Color c(255, 255, 255);
			//	//this->drawTriangle(screen_coords[0], screen_coords[1], screen_coords[2], c);

			//	// 包括纹理
			//	this->drawTriangle(screen_coords[0], screen_coords[1], screen_coords[2],
			//		uv[0], uv[1], uv[2], 1);
			//}
			// Gouroud shading
			if (intensity[0] > 0 || intensity[1] > 0 || intensity[2] > 0)
				this->drawTriangle(screen_coords[0], screen_coords[1], screen_coords[2],
								uv[0], uv[1], uv[2],
								intensity[0], intensity[1], intensity[2]);
		}
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

Vector3f Renderer::modelTransform(const Vector3f &p, const Matrix4f& transformMatrix)
{
	Vector4f p_h(p.x(), p.y(), p.z(), 1);
	Vector4f p_after = transformMatrix * p_h;
	//return Vector3f(p_after.x() / p_after[3], p_after.y() / p_after[3], p_after.z() / p_after[3]);
	return Vector3f(p_after.x(), p_after.y(), p_after.z());
}

Vector3f Renderer::modelTransform(const Vector3f &p, const Matrix3f &rotation, const Vector3f &translate)
{
	return rotation * p + translate;
}

Vector3f Renderer::transform(const Vector3f &p, const Matrix4f &transformMatrix)
{
	Vector4f p_h(p.x(), p.y(), p.z(), 1);
	Vector4f p_after = transformMatrix * p_h;
	return Vector3f(p_after.x(), p_after.y(), p_after.z());
}
