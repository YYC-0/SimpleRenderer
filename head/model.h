#pragma once

#include <vector>
#include <string>
#include <Eigen/Core>
#include "Color.h"
#include "tgaimage.h"
//#include "geometry.h"
using namespace Eigen;

class Model {
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

	Model(const std::string& filename);
	~Model();
	int nverts() const;
	int nfaces() const;
	Vector3f vert(const int& i) const;
	Vector3f vert(const int &iface, const int &nthvert) const;
	Vector2i uv(const int &iface, const int &nvert);
	Vector3f normal(const int &iface, const int &nvert);
	Vector3f normal(const Vector2i &uv);
	Color diffuse(const Vector2i &uv);
	Color diffuse(const Vector2f &uv);
	//Color specular(Vector2i uv);
	float specular(const Vector2i &uv);
	std::vector<int> face(int idx) const;

private:
	std::vector<Vector3f> verts_;
	std::vector<std::vector<Vector3i>> faces_; // vector3i means vertex/uv/normal
	std::vector<Vector3f> norms;
	std::vector<Vector2f> uv_;
	TGAImage diffusemap_;
	TGAImage normalmap_;
	TGAImage specularmap_;
	void load_texture(std::string fileName, const char *suffix, TGAImage &img);
};

