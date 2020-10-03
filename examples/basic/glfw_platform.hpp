#pragma once

static void fb_size_cb(GLFWwindow* window, int width, int height);

struct GLFWPlatform : public Vulkan::WSIPlatform
{

	GLFWPlatform()
	{
		width = 1280;
		height = 720;
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		window = glfwCreateWindow(width, height, "GLFW Window", nullptr, nullptr);

		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, fb_size_cb);
	}

	virtual ~GLFWPlatform() 
	{
		if (window)
			glfwDestroyWindow(window);
	}

	virtual VkSurfaceKHR CreateSurface(VkInstance instance, VkPhysicalDevice gpu)
	{
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
			return VK_NULL_HANDLE;

		int actual_width, actual_height;
		glfwGetFramebufferSize(window, &actual_width, &actual_height);
		width = unsigned(actual_width);
		height = unsigned(actual_height);
		return surface;
	}

	virtual std::vector<const char*> GetInstanceExtensions()
	{
		uint32_t count;
		const char** ext = glfwGetRequiredInstanceExtensions(&count);
		return { ext, ext + count };
	}

	virtual uint32_t GetSurfaceWidth()
	{
		return width;
	}

	virtual uint32_t GetSurfaceHeight()
	{
		return height;
	}

	virtual bool Alive(Vulkan::WSI& wsi)
	{
		return !glfwWindowShouldClose(window);
	}

	virtual void PollInput()
	{
		glfwPollEvents();
	}

	void NotifyResize(int width_, int height_)
	{
		resize = true;
		width = static_cast<uint32_t>(width_);
		height = static_cast<uint32_t>(height_);
	}

private:

	GLFWwindow* window = nullptr;
	unsigned width = 0;
	unsigned height = 0;

};

static void fb_size_cb(GLFWwindow* window, int width, int height)
{
	auto* glfw = static_cast<GLFWPlatform*>(glfwGetWindowUserPointer(window));
	glfw->NotifyResize(width, height);
}
