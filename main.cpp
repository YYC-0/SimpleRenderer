#include <iostream>
#include <string>
#include <vector>
#include <chrono>

#include "head/model.h"
#include "head/Renderer.h"
#include "head/Camera.h"
#include "head/Shader.h"
#include "head/Window.h"
#include <Eigen/Core>
#include <Eigen/Geometry>
using namespace std;
using namespace std::chrono;

const int WIDTH = 800;
const int HEIGHT = 800;

int main()
{
	TCHAR *title = _T("SimpleRenderer | WASD移动视角, QE缩放");
	if (screen_init(WIDTH, HEIGHT, title))
		return -1;
	init(WIDTH, HEIGHT, screen_fb);

	string model_path = "obj/african_head/african_head.obj";
	//string model_path = "obj/diablo3_pose/diablo3_pose.obj";
	Vector3f lightPos = Vector3f(1, 1, 1);
	Camera *camera = new Camera();
	Model model(model_path);
	Renderer renderer(WIDTH, HEIGHT, frameBuffer, camera, lightPos);

	while (screen_exit == 0 && screen_keys[VK_ESCAPE] == 0)
	{
		auto start = steady_clock::now();
		screen_dispatch();
		renderer.bufferClear();

		if (screen_keys[VK_RIGHT] || screen_keys['D'])
			camera->move(Camera::Action::RIGHT);
		if (screen_keys[VK_LEFT] || screen_keys['A'])
			camera->move(Camera::Action::LEFT);
		if (screen_keys[VK_UP] || screen_keys['W'])
			camera->move(Camera::Action::UP);
		if (screen_keys[VK_DOWN] || screen_keys['S'])
			camera->move(Camera::Action::DOWN);
		if (screen_keys['Q'])
			camera->move(Camera::Action::ZOOM_IN);
		if (screen_keys['E'])
			camera->move(Camera::Action::ZOOM_OUT);

		Matrix4f modelMatrix = Matrix4f::Identity();
		renderer.drawModel(&model, Renderer::DrawMode::TRIANGLE, modelMatrix);
		screen_update();
		auto end = steady_clock::now();
		double fps = 1.0 / duration<double>(end - start).count();
		cout << "FPS: " << fps << '\r';
	}

	delete camera;
	return 0;
}
