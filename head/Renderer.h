#pragma once
#include "model.h"

class Color
{
public:
	Color() : r(0), g(0), b(0), hex(0) {}
	Color(unsigned int r_, unsigned int g_, unsigned int b_) :
		r(r_), g(g_), b(b_) {
		hex = b + (g << 8) + (r << 16);
	}

	unsigned int r;
	unsigned int g;
	unsigned int b;
	unsigned int hex;
};

class Renderer
{
public:
	enum class DrawMode {
		LINE,
		TRIANGLE
	};

	Renderer() = default;
	Renderer(int w, int h, unsigned int** fb);

	void set(const int& x, const int& y, const Color& c); // set pixel with color c
	void drawLine(int x0, int y0, int x1, int y1, const Color& c);
	void drawLine(Vector2i t0, Vector2i t1, const Color& c);
	void drawTriangle(Vector2i t0, Vector2i t1, Vector2i t2, const Color& c);
	void drawModel(const Model& model, DrawMode mode);

private:
	int width;
	int height;
	unsigned int** frameBuffer;
	Vector3f light_dir;

	inline bool isLegal(const int& x, const int& y) {
		if (x < 0 || x >= width || y < 0 || y >= height) return false;
		return true;
	}

	// return >0 if p left of line l0-l1
	// ==0 if p on the line
	// <0 if p right
	int isLeft(Vector2i l0, Vector2i l1, Vector2i p);
	bool isInTriangle(const Vector2i& v0, const Vector2i& v1, const Vector2i& v2, const Vector2i& p); // 判断点p是否在三角形内

};

