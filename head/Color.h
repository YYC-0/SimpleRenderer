#pragma once

class Color
{
public:
	Color() : r(0), g(0), b(0), hex(0) {}
	Color(unsigned int r_, unsigned int g_, unsigned int b_) :
		r(r_), g(g_), b(b_) {
		hex = b + (g << 8) + (r << 16);
	}
	inline Color& operator*(float k);
	inline bool operator==(Color &c);

	unsigned int r;
	unsigned int g;
	unsigned int b;
	unsigned int hex;
};

inline Color& Color::operator*(float k)
{
	r *= k;
	g *= k;
	b *= k;
	if (r > 255) r = 255;
	if (r < 0) r = 0;
	if (g > 255) g = 255;
	if (g < 0) g = 0;
	if (b > 255) b = 255;
	if (b < 0) b = 0;
	hex = b + (g << 8) + (r << 16);
	return *this;
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
