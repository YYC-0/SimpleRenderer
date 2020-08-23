#pragma once

class Color
{
public:
	Color() : r(0), g(0), b(0), hex(0) {}
	Color(unsigned int r_, unsigned int g_, unsigned int b_) :
		r(r_), g(g_), b(b_) {
		if (r > 255) r = 255;
		if (r < 0) r = 0;
		if (g > 255) g = 255;
		if (g < 0) g = 0;
		if (b > 255) b = 255;
		if (b < 0) b = 0;
		hex = b + (g << 8) + (r << 16);
	}
	inline Color operator*(float k);
	inline Color operator+(const Color &c);
	inline bool operator==(Color &c);

	unsigned int r;
	unsigned int g;
	unsigned int b;
	unsigned int hex;
};

inline Color Color::operator*(float k)
{
	unsigned int newR = r * k;
	unsigned int newG = g * k;
	unsigned int newB = b * k;
	if (newR > 255) newR = 255;
	if (newR < 0) newR = 0;
	if (newG > 255) newG = 255;
	if (newG < 0) newG = 0;
	if (newB > 255) newB = 255;
	if (newB < 0) newB = 0;
	return Color(newR, newG, newB);
}

inline Color Color::operator+(const Color &c)
{
	unsigned int newR = r + c.r;
	unsigned int newG = g + c.g;
	unsigned int newB = b + c.b;
	if (newR > 255) newR = 255;
	if (newR < 0) newR = 0;
	if (newG > 255) newG = 255;
	if (newG < 0) newG = 0;
	if (newB > 255) newB = 255;
	if (newB < 0) newB = 0;
	return Color(newR, newG, newB);
}

inline bool Color::operator==(Color &c)
{
	if (this->r == c.r &&
		this->g == c.g &&
		this->b == c.b &&
		this->hex == c.hex)
		return true;
	return false;
}
