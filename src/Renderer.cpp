#include "../head/Renderer.h"

#include <cmath>
#include <algorithm>
#include <vector>
#include <Eigen/Core>
#include <Eigen/Dense>
#include <iostream>

Renderer::Renderer(int w, int h, unsigned int** fb, float **zbuf, Camera *camera, Vector3f lightDir)
{
	width = w;
	height = h;
	frameBuffer_ = fb;
	zBuffer = zbuf;
	lightDir_ = lightDir;
	camera_ = camera;

	zNear_ = -0.1;
	zFar_ = -100.0;
	FOV_ = M_PI / 4.0;


	initProjectionMatrix();
	cout << "View Matrix:\n" << camera_->getViewMatrix() << endl;
	cout << "Projection Matrix:\n" << projectionMatrix_ << endl;
	//projectionMatrix_ = Matrix4f::Identity();
	//float c = camera_->getPos().norm();
	//projectionMatrix_(3, 2) = -1.0 / c;
	viewPortMatrix_ <<
		width/2,0,			0,		 width/2,
		0,		height/2,	0,		 height/2,
		0,		0,			1.0/2.0, 1.0/2.0,
		0,		0,			0,		 1;

	depthMap = new unsigned int *[height];
	shadowBuffer = new float *[height];
	for (int i = 0; i < height; ++i)
	{
		depthMap[i] = new unsigned int[width];
		shadowBuffer[i] = new float[width];
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
	}
	delete shader;
	delete depthShader;
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
void Renderer::drawTriangle_gouraud(Vector3f t0, Vector3f t1, Vector3f t2,
							Vector3f n0, Vector3f n1, Vector3f n2,
							Vector2i uv0, Vector2i uv1, Vector2i uv2,
							Matrix4f normalMatrix, Matrix3f TBN)
{		
	int max_x = min((int)max(max(t0.x(), t1.x()), t2.x()), width-1);
	int min_x = max((int)min(min(t0.x(), t1.x()), t2.x()), 0);
	int max_y = min((int)max(max(t0.y(), t1.y()), t2.y()), height-1);
	int min_y = max((int)min(min(t0.y(), t1.y()), t2.y()), 0);
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

				Color objectColor = model->diffuse(Vector2i(uvP.x(), uvP.y()));

				// 计算法线空间中的光线方向
				Vector3f NAB = n1 - n0,
					NAC = n2 - n0;
				Vector3f triN = n0 + v * NAB + u * NAC;
				triN.normalize();
				Vector3f N = transform(triN, normalMatrix);
				N.normalize();
				TBN.row(2) = N;
				Vector3f n = model->normal(Vector2i(uvP.x(), uvP.y())); // 法线贴图法线
				Vector3f light = TBN * lightDir_;
				float intensity = max(n.dot(light), 0.2f);

				Color color = objectColor * intensity;
				float z = p3f.z();
				if (isLegal(x, height - y) && z > zBuffer[height - y][x])
				{
					setZBuffer(x, y, z);
					set(x, y, color);
					//set(x, y, Color(255,255,255) * ((z-zNear_) / (zFar_-zNear_)));
				}
			}
		}
}

// draw triangle with normal map
// Phong's lighting model
// 错误
void Renderer::drawTriangle(Vector3f t0, Vector3f t1, Vector3f t2, 
	Vector2i uv0, Vector2i uv1, Vector2i uv2)
{
	int max_x = min((int)max(max(t0.x(), t1.x()), t2.x()), width - 1);
	int min_x = max((int)min(min(t0.x(), t1.x()), t2.x()), 0);
	int max_y = min((int)max(max(t0.y(), t1.y()), t2.y()), height - 1);
	int min_y = max((int)min(min(t0.y(), t1.y()), t2.y()), 0);
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

				Color objectColor = model->diffuse(Vector2i(uvP.x(), uvP.y()));
				Vector3f n = model->normal(Vector2i(uvP.x(), uvP.y()));
				Vector3f r = n * n.dot(lightDir_) * 2.0 - lightDir_; // reflected light
				r.normalize();
				float spec = pow(max(r.z(), 0.0f), model->specular(Vector2i(uvP.x(), uvP.y())));
				float diff = max(0.0f, n.dot(lightDir_));
				float intensity = max(n.dot(lightDir_), 0.3f);
				Color color = objectColor * (diff + 0.6 * spec);
				float z = p3f.z();
				if (z > zBuffer[height - y][x])
				{
					zBuffer[height - y][x] = z;
					set(x, y, color);
				}
			}
		}
}

void Renderer::drawTriangle(Vector3f screenCoords[3], FShader *shader, unsigned int **frameBuffer, float **zBuffer)
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
					Color color = shader->fragment(uv);
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
	Matrix4f viewMatrix = camera_->getViewMatrix();
	Matrix4f MVP = projectionMatrix_ * viewMatrix * modelMatrix;

#pragma omp parallel for
	for (int i = 0; i < model->nfaces(); ++i)
	{
		std::vector<int> face = model->face(i);

		Vector3f screen_coords[3];
		Vector3f world_coords[3];
		Vector3f NDC[3];
		Vector2i uv[3];
		Vector3f normal[3];
		float intensity[3];
		for (int j = 0; j < 3; ++j)
		{
			// get vertex
			Vector3f v = model->vert(face[j]);
			world_coords[j] = v;
			// coordnate transform
			NDC[j] = transform(v, MVP);
			screen_coords[j] = transform(NDC[j], viewPortMatrix_);
			//screen_coords[j] = worldToScreen(model_coords[j]);
			//intensity[j] = max(model->normal(i, j).dot(lightDir_), 0.3f);
			uv[j] = model->uv(i, j);
			normal[j] = model->normal(i, j);
		}
		// 丢弃不在视口中的图元
		if(!isInView(NDC[0]) && !isInView(NDC[1]) && !isInView(NDC[2]))
			continue;

		if (mode == DrawMode::LINE)
		{
			for (int j = 0; j < 3; ++j)
			{
				this->drawLine(screen_coords[j].x(), screen_coords[j].y(),
					screen_coords[(j + 1) % 3].x(), screen_coords[(j + 1) % 3].y(), Color(255, 255, 255));
			}
		}
		else if (mode == DrawMode::TRIANGLE)
		{
			//Vector3f n = (world_coords[1] - world_coords[0]).cross(world_coords[2] - world_coords[1]); // 三角形法线
			//n.normalize();
			//float intensity = n.dot(lightDir_);
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
			Matrix4f normalMatrix = modelMatrix.inverse().transpose();
			// compute normal
			//Vector3f triN = (world_coords[1] - world_coords[0]).cross(world_coords[2] - world_coords[1]);
			//Vector3f triN = (normal[0] + normal[1] + normal[2]);
			//triN.normalize();
			// 计算TBN矩阵
			Vector3f tangent, bitangent;
			Vector3f edge1 = world_coords[1] - world_coords[0], 
				edge2 = world_coords[2] - world_coords[0];
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
			//Vector3f N = transform(triN, normalMatrix);
			T.normalize(), B.normalize();// , N.normalize();
			Matrix3f TBN;
			TBN.row(0) = T;
			TBN.row(1) = B;
			//TBN.row(2) = N;

			this->drawTriangle_gouraud(screen_coords[0], screen_coords[1], screen_coords[2],
								normal[0], normal[1], normal[2],
								uv[0], uv[1], uv[2], normalMatrix, TBN);
			// normal map
			//this->drawTriangle(screen_coords[0], screen_coords[1], screen_coords[2], uv[0], uv[1], uv[2]);
		}
	}
}

void Renderer::drawModel_shader(Model *model, DrawMode mode, Matrix4f modelMatrix)
{
	this->model = model;
	Matrix4f cameraProjectionMatrix = computeProjectionMatrix(width, height, M_PI / 2, zNear_, zFar_);
	Camera lightCamera(lightDir_);

	shader->setModel(model);
	Matrix4f cameraT = viewPortMatrix_ * cameraProjectionMatrix * lightCamera.getViewMatrix() * modelMatrix;
	shader->setMatrix(modelMatrix, projectionMatrix_, camera_->getViewMatrix(), cameraT);

	depthShader->setModel(model);
	depthShader->setMatrix(modelMatrix, cameraProjectionMatrix, lightCamera.getViewMatrix());
	Vector3f screenCoords[3];

	// render shadow map
	for (int i = 0; i < model->nfaces(); ++i)
	{
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
		drawTriangle(screenCoords, depthShader, depthMap, shadowBuffer);
	}

//#pragma omp parallel for
	for (int i = 0; i < model->nfaces(); ++i)
	{
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
		shader->computeTBN();
		drawTriangle(screenCoords, shader, frameBuffer_, zBuffer);
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


Vector3f Renderer::modelTransform(const Vector3f &p, const Matrix3f &rotation, const Vector3f &translate)
{
	return rotation * p + translate;
}

Vector3f Renderer::transform(const Vector3f &p, const Matrix4f &transformMatrix)
{
	Vector4f p_h(p.x(), p.y(), p.z(), 1);
	Vector4f p_after = transformMatrix * p_h;
	return Vector3f(p_after.x() / p_after[3], p_after.y() / p_after[3], p_after.z() / p_after[3]);
	//return Vector3f(p_after.x(), p_after.y(), p_after.z());
}
Vector3f Renderer::transform2(const Vector3f &p, const Matrix4f &transformMatrix)
{
	Vector4f p_h(p.x(), p.y(), p.z(), 1);
	Vector4f p_after = transformMatrix * p_h;
	//return Vector3f(p_after.x() / p_after[3], p_after.y() / p_after[3], p_after.z() / p_after[3]);
	return Vector3f(p_after.x(), p_after.y(), p_after.z());
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
