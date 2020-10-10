#include <quantumvk/quantumvk.hpp>

#include <iostream>
#include <thread>

#include <GLFW/glfw3.h>

#include "../common/glfw_platform.hpp"
#include "../common/file_loader.hpp"

int main(int argc, char** argv)
{
	glfwInit();

	if (!Vulkan::Context::InitLoader(nullptr))
		QM_LOG_ERROR("Failed to load vulkan dynamic library");

	{
		GLFWPlatform platform;

		Vulkan::WSI wsi;
		wsi.SetPlatform(&platform);
		wsi.SetBackbufferSrgb(true);
		wsi.Init(1, nullptr, 0);

		{
			Vulkan::Device& device = wsi.GetDevice();
			
			std::vector<char> vertex_code = ReadFile("spirv/vertex.spv");
			std::vector<char> frag_code = ReadFile("spirv/fragment.spv");
			
			Vulkan::ShaderHandle vert_shader = device.CreateShader(reinterpret_cast<const uint32_t*>(vertex_code.data()), vertex_code.size());
			Vulkan::ShaderHandle frag_shader = device.CreateShader(reinterpret_cast<const uint32_t*>(frag_code.data()), frag_code.size());
			
			Vulkan::GraphicsProgramShaders p_shaders;
			p_shaders.vertex = vert_shader;
			p_shaders.fragment = frag_shader;

			Vulkan::ProgramHandle program = device.CreateGraphicsProgram(p_shaders);
			
			float current_time = 0;
			float current_delta = 1.0f/60.0f;
			float current_hue = 0.1f;
			float current_target = 0.1f;
			
			std::srand(100);
			
			while (platform.Alive(wsi))
			{

				Util::Timer timer;
				timer.start();

				wsi.BeginFrame();
				{
					
					if ((float)std::rand() / (float)RAND_MAX > .993f)
					{
						QM_LOG_TRACE("Randomizing color...\n");
						current_target = (float)std::rand() / (float)RAND_MAX;
					}

					float dist = current_target - current_hue;
					
					current_hue += dist * current_delta * 0.5f;
					
					// Rendering process
					
					auto cmd = device.RequestCommandBuffer();

					// Just render a clear color to screen.
					// There is a lot of stuff going on in these few calls which will need its own sample to explore w.r.t. synchronization.
					// For now, you'll just get a blue-ish color on screen.
					Vulkan::RenderPassInfo rp = device.GetSwapchainRenderPass(Vulkan::SwapchainRenderPass::ColorOnly);
					rp.color_attachments[0].clear_color.float32[0] = 0.1f;
					rp.color_attachments[0].clear_color.float32[1] = 0.2f;
					rp.color_attachments[0].clear_color.float32[2] = 0.3f;
					cmd->BeginRenderPass(rp);

					cmd->SetOpaqueState();

					cmd->SetProgram(program);
					cmd->SetVertexAttrib(0, 0, VK_FORMAT_R32G32_SFLOAT, 0);
					cmd->SetVertexBinding(0, sizeof(float) * 2);
					cmd->SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
					cmd->SetCullMode(VK_CULL_MODE_NONE);

					float cpu_vert_data[] = { -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f };
					void* gpu_vert_data = cmd->AllocateVertexData(0, sizeof(cpu_vert_data));
					memcpy(gpu_vert_data, cpu_vert_data, sizeof(cpu_vert_data));

					float cpu_uniform_data[] = { current_hue, 0.3f , current_time / 10.0f, current_time };
					void* gpu_uniform_data = cmd->AllocateConstantData(0, 0, 0, sizeof(float) * 4);
					memcpy(gpu_uniform_data, cpu_uniform_data, sizeof(float) * 4);

					cmd->Draw(6);

					cmd->EndRenderPass();
					device.Submit(cmd);
					
					// -----------------
				}

				wsi.EndFrame();

				current_delta = timer.end();
				current_time += current_delta;


				//QM_LOG_INFO("Frame time (ms): %f\n", time_milli);
			}

			program.Reset();
			device.WaitIdle();
		}


		QM_LOG_TRACE("Detroying WSI\n");
	}

	glfwTerminate();
}