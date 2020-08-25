#pragma once
#include <Eigen/Core>
#include <map>
#include <string>
#include "Color.h"
#include "model.h"
using namespace Eigen;
using namespace std;

class FShader
{
public:
	virtual ~FShader() {};
	virtual Vector3f vertex(int iface, int nthvert) = 0;
	virtual Color fragment(std::pair<float, float> barycentricUV) = 0;

protected:
	Vector3f transform(const Vector3f &p, const Matrix4f &transformMatrix){
		Vector4f p_h(p.x(), p.y(), p.z(), 1);
		Vector4f p_after = transformMatrix * p_h;
		return Vector3f(p_after.x() / p_after[3], p_after.y() / p_after[3], p_after.z() / p_after[3]);
	}
	Vector3f reflect(Vector3f v, Vector3f n) {
		// 由入射光v和表面法线计算反射向量
		float project = v.dot(n);
		Vector3f N = project * n;
		Vector3f reflectVector = v - 2 * N;
		reflectVector.normalize();
		return reflectVector;
	}
};

// 预先定义的一些shader，也可添加其他shader

// Phong's lighting model
// 使用了法向空间法线贴图
// 使用了阴影映射
class Shader : public FShader
{
public:
	Matrix4f T; // viewport matrix * projection matrix * view matrix * model matrix
	Matrix4f normalMatrix; // model matrix inverse transpose
	Matrix4f viewPortMatrix;
	Matrix4f matrixShadow; // transform framebuffer screen coordinates to shadowbuffer screen coordinates
	Vector2i uv[3];
	Vector3f worldCoords[3];
	Vector3f screenCoords[3];
	Vector3f normal[3];
	Matrix3f TBN;
	Model *model;
	Vector3f lightDir;
	float **shadowBuffer;
	int height;
	int width;
	Vector3f cameraPos;

	Shader(Matrix4f viewPort, Vector3f lightD, float **shadow, int h, int w) :
		viewPortMatrix(viewPort), 
		model(nullptr), 
		lightDir(lightD),
		shadowBuffer(shadow),
		height(h), width(w) 
	{ 
		TBN = Matrix3f::Zero();
	}

	virtual Vector3f vertex(int iface, int nthvert)
	{
		worldCoords[nthvert] = model->vert(iface, nthvert);
		uv[nthvert] = model->uv(iface, nthvert);
		normal[nthvert] = model->normal(iface, nthvert);
		screenCoords[nthvert] = transform(worldCoords[nthvert], T);
		return screenCoords[nthvert];
	}

	virtual Color fragment(std::pair<float, float> barycentricUV)
	{
		// 计算当前像素对应的各个向量（不应该在这个函数中）
		float u = barycentricUV.first, v = barycentricUV.second;
		Vector2i uvAB = uv[1] - uv[0],
			uvAC = uv[2] - uv[0];
		Vector2f uvP = uv[0].cast<float>() + v * uvAB.cast<float>() + u * uvAC.cast<float>(); // u v ？？

		Vector3f AB = screenCoords[1] - screenCoords[0],
			AC = screenCoords[2] - screenCoords[0]; 
		Vector3f p3f = screenCoords[0] + v * AB + u * AC; // u v ？？
		Vector3f shadowP = transform(p3f, matrixShadow);

		Vector3f WAB = worldCoords[1] - worldCoords[0],
			WAC = worldCoords[2] - worldCoords[0];
		Vector3f worldPos = worldCoords[0] + v * AB + u * AC; // u v ？？

		// 计算法向量
		Vector3f NAB = normal[1] - normal[0],
			NAC = normal[2] - normal[0];
		Vector3f triN = normal[0] + v * NAB + u * NAC;
		triN.normalize();
		Vector3f N = transform(triN, normalMatrix);
		N.normalize();
		TBN.row(2) = N;
		Vector3f n = model->normal(Vector2i(uvP.x(), uvP.y())); // 法线贴图法线
		Vector3f light = TBN * lightDir; // 将光线方向转换到法向空间
		light.normalize();

		// compute color
		// phong
		Color objectColor = model->diffuse(Vector2i(uvP.x(), uvP.y()));
		float ambient = 0.3;
		float diff = std::max(n.dot(light), 0.0f);
		float specularStrength = 0.5f;
		Vector3f viewDir = cameraPos - worldPos;
		viewDir.normalize();
		Vector3f reflectDir = (-light, n);
		float spec = pow(max(viewDir.dot(reflectDir), 0.0f), 16);
		float specular = specularStrength * spec;

		float shadowDepth = shadowBuffer[height - (int)shadowP.y()][(int)shadowP.x()];
		float shadow = shadowDepth > shadowP.z()*shadowP.z() + std::max(0.001f, 0.02f*(1.0f-n.dot(light))) ? 1.0 : 0.0;
		
		Color color = objectColor * (ambient + (1.0 - shadow) * (diff + specular));
		return color;
	}

	void setModel(Model *m) { model = m; }
	void setCameraPos(Vector3f pos) { cameraPos = pos; }
	void setMatrix(const Matrix4f &modelMatrix, const Matrix4f &projectionMatrix, const Matrix4f &viewMatrix, const Matrix4f &cameraT)
	{
		T = viewPortMatrix * projectionMatrix * viewMatrix * modelMatrix;
		normalMatrix = modelMatrix.inverse().transpose();
		matrixShadow = cameraT * T.inverse();
	}
	// 计算TBN矩阵的TB
	void computeTBN()
	{
		Vector3f tangent, bitangent;
		Vector3f edge1 = worldCoords[1] - worldCoords[0],
			edge2 = worldCoords[2] - worldCoords[0];
		Vector2i deltaUV1 = uv[1] - uv[0], deltaUV2 = uv[2] - uv[0];
		float f = 1.0 / (deltaUV1.x() * deltaUV2.y() - deltaUV2.x() * deltaUV1.y());
		tangent << f * (deltaUV2.y() * edge1.x() - deltaUV1.y() * edge2.x()),
			f *(deltaUV2.y() * edge1.y() - deltaUV1.y() * edge2.y()),
			f *(deltaUV2.y() * edge1.z() - deltaUV1.y() * edge2.z());
		tangent.normalize();
		bitangent << f * (-deltaUV2.x() * edge1.x() + deltaUV1.x() * edge2.x()),
			f *(-deltaUV2.x() * edge1.y() + deltaUV1.x() * edge2.y()),
			f *(-deltaUV2.x() * edge1.z() + deltaUV1.x() * edge2.z());
		Vector3f T = transform(tangent, normalMatrix);
		Vector3f B = transform(bitangent, normalMatrix);
		T.normalize(), B.normalize();
		TBN.row(0) = T;
		TBN.row(1) = B;
	}
};

// 绘制深度图
// 用于阴影映射
class DepthShader : public FShader
{
public:
	Matrix4f MVP;
	Matrix4f viewPortMatrix;
	Vector3f screenCoords[3];
	Model *model;

	DepthShader(Matrix4f viewPort) :
		viewPortMatrix(viewPort),
		model(nullptr) { }

	virtual Vector3f vertex(int iface, int nthvert)
	{
		Vector3f v = model->vert(iface, nthvert);
		screenCoords[nthvert] = transform(v, viewPortMatrix * MVP);
		return screenCoords[nthvert];
	}

	virtual Color fragment(std::pair<float, float> barycentricUV)
	{
		//float u = barycentricUV.first, v = barycentricUV.second;
		//Vector3f AB = screenCoords[1] - screenCoords[0],
		//	AC = screenCoords[2] - screenCoords[0];
		//Vector3f p3f = screenCoords[0] + v * AB + u * AC; // u v ？？
		//return Color(255, 255, 255) * (p3f.z()-1.0) * 2;
		return Color();
	}

	void setModel(Model *m) { model = m; }

	void setMatrix(const Matrix4f &modelMatrix, const Matrix4f &projectionMatrix, const Matrix4f &viewMatrix)
	{
		MVP = projectionMatrix * viewMatrix * modelMatrix;
	}
};