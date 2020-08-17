#include "../head/Renderer.h"

#include <cmath>
#include <algorithm>
#include <vector>
#include <Eigen/Core>
#include <Eigen/Dense>
#include <iostream>
using namespace std;

Renderer::Renderer(int w, int h, unsigned int** fb)
{
	width = w;
	height = h;
	frameBuffer = fb;
	light_dir = Vector3f(0, 0, -1);
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

void Renderer::drawModel(const Model& model, DrawMode mode)
{
	for (int i = 0; i < model.nfaces(); ++i)
	{
		std::vector<int> face = model.face(i);
		if (mode == DrawMode::LINE)
		{
			for (int j = 0; j < 3; ++j)
			{
					Vector3f v0 = model.vert(face[j]);
					Vector3f v1 = model.vert(face[(j + 1) % 3]);
					int x0 = (v0.x() + 1.0) * width / 2; // obj 中是-1到1
					int y0 = (v0.y() + 1.0) * height / 2.;
					int x1 = (v1.x() + 1.0) * width / 2.;
					int y1 = (v1.y() + 1.0) * height / 2.;
					this->drawLine(x0, y0, x1, y1, Color());
			}
		}
		else if (mode == DrawMode::TRIANGLE)
		{
			Vector2i screen_coords[3];
			Vector3f world_coords[3];
			for (int j = 0; j < 3; ++j)
			{
				Vector3f v = model.vert(face[j]);
				screen_coords[j] = Vector2i((v.x() + 1.) * width / 2., (v.y() + 1.) * height / 2.);
				world_coords[j] = v;
			}
			// 三角形法线
			Vector3f n = (world_coords[2] - world_coords[1]).cross(world_coords[1] - world_coords[0]);
			n.normalize();
			float intensity = n.dot(light_dir);
			if (intensity > 0)
			{
				Color c(255.0 * intensity, 255.0 * intensity, 255.0 * intensity);
				this->drawTriangle(screen_coords[0], screen_coords[1], screen_coords[2], c);
			}
		}
	}
}

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
	int abab = ab.dot(ab),
		abac = ab.dot(ac),
		acab = ac.dot(ab),
		acac = ac.dot(ac),
		apac = ap.dot(ac),
		apab = ap.dot(ab);
	int denominator = acac * abab - acab * abac;
	double u = (double)(abab * apac - acab * apab) / (double)denominator;
	double v = (double)(acac*apab-acab*apac) / (double)denominator;
	if (u >= 0 && v >= 0 && u + v <= 1)
		return true;
	return false;
}
