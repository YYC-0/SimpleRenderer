#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include <string>
#include <Eigen/Core>
//#include "geometry.h"
using namespace Eigen;

class Model {
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

	Model(const std::string& filename);
	~Model();
	int nverts() const;
	int nfaces() const;
	Vector3f vert(int i) const;
	std::vector<int> face(int idx) const;

private:
	std::vector<Vector3f, Eigen::aligned_allocator<Vector3f>> verts_;
	std::vector<std::vector<int> > faces_;
};

#endif //__MODEL_H__
