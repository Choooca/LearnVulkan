#pragma once
#include <render/window.h>

class Application {

public:
	static constexpr int WIDTH = 800;
	static constexpr int HEIGHT = 600;

	void Loop();

private:
	Window window{WIDTH, HEIGHT, "HELLO VULKAN" };
};