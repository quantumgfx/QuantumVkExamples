#include <quantumvk/quantumvk.hpp>

#include <iostream>
#include <thread>

#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS

#include <glm/gtc/matrix_transform.hpp>

#include "../common/glfw_platform.hpp"
#include "../common/file_loader.hpp"

static bool is_mouse_pressed = false;
static double mouse_x = 0, mouse_y = 0;

static inline void SetCameraMouseCallbacks(GLFWwindow* window)
{
	glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods)
		{
			switch (action)
			{
			case GLFW_PRESS:
			{
				is_mouse_pressed = true;
				break;
			}
			case GLFW_RELEASE:
			{
				is_mouse_pressed = false;
				break;
			}
			}
		});

	/*glfwSetScrollCallback(window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			data.eventCallback(event);
		})*/;

	glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xPos, double yPos)
		{
			mouse_x = xPos; 
			mouse_y = yPos;
		});
}

int main(int argc, char** argv)
{

	char* obj_file = "model.obj";
	char* diffuse_file = "diffuse.png";

	if (argc == 3)
	{
		obj_file = argv[1];
		diffuse_file = argv[2];
	}
	
	std::cout << "Using obj file " << obj_file << "\n";
	std::cout << "Using diffuse texture " << diffuse_file << "\n";


	glfwInit();

	if (!Vulkan::Context::InitLoader(nullptr))
		QM_LOG_ERROR("Failed to load vulkan dynamic library");

	{
		GLFWPlatform platform;

		SetCameraMouseCallbacks(platform.GetNativeWindow());

		Vulkan::WSI wsi;
		wsi.SetPlatform(&platform);
		wsi.SetBackbufferSrgb(true);
		wsi.Init(1, nullptr, 0);

		{
			Vulkan::Device& device = wsi.GetDevice();
			
			std::vector<char> vertex_code = ReadFile("spirv/vertex.spv");
			std::vector<char> frag_code = ReadFile("spirv/fragment.spv");
			
			Vulkan::ShaderHandle vert_shader = device.CreateShader(vertex_code.size() / sizeof(uint32_t), reinterpret_cast<const uint32_t*>(vertex_code.data()));
			Vulkan::ShaderHandle frag_shader = device.CreateShader(frag_code.size() / sizeof(uint32_t), reinterpret_cast<const uint32_t*>(frag_code.data()));
			
			Vulkan::GraphicsProgramShaders p_shaders;
			p_shaders.vertex = vert_shader;
			p_shaders.fragment = frag_shader;

			Vulkan::ProgramHandle program = device.CreateGraphicsProgram(p_shaders);

			Vulkan::BufferHandle vertex_buffer;
			Vulkan::BufferHandle index_buffer;

			uint32_t index_count;

			std::cout << "Loading model\n";
			
			// Create vertex and index buffers
			{
				std::vector<Vertex> vertices;
				std::vector<uint32_t> indices;

				LoadObjModel(obj_file, vertices, indices);

				std::cout << "Model has " << vertices.size() << " vertices, and " << indices.size() << " indices\n";

				index_count = indices.size();

				Vulkan::BufferCreateInfo vert_create_info{};
				vert_create_info.domain = Vulkan::BufferDomain::Device;
				vert_create_info.size = sizeof(Vertex) * vertices.size();
				vert_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
				vert_create_info.sharing_mode = Vulkan::BufferSharingMode::Exclusive;
				vert_create_info.exclusive_owner = Vulkan::BUFFER_COMMAND_QUEUE_GENERIC;

				vertex_buffer = device.CreateBuffer(vert_create_info, vertices.data());

				Vulkan::BufferCreateInfo index_create_info{};
				index_create_info.domain = Vulkan::BufferDomain::Device;
				index_create_info.size = sizeof(uint32_t) * indices.size();
				index_create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
				index_create_info.sharing_mode = Vulkan::BufferSharingMode::Exclusive;
				index_create_info.exclusive_owner = Vulkan::BUFFER_COMMAND_QUEUE_GENERIC;

				index_buffer = device.CreateBuffer(index_create_info,  indices.data());
			}

			std::cout << "Loading diffuse texture\n";

			Vulkan::ImageHandle diffuse;
			Vulkan::ImageViewHandle diffuse_view;

			{
				int width, height;
				std::vector<unsigned char> pixels = LoadTexture(diffuse_file, width, height);

				std::cout << "Texture has width: " << width << " and height " << height << "\n";

				Vulkan::ImageCreateInfo diffuse_create_info = Vulkan::ImageCreateInfo::Immutable2dImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, true);
				diffuse_create_info.sharing_mode = Vulkan::ImageSharingMode::Exclusive;
				diffuse_create_info.exclusive_owner = Vulkan::IMAGE_COMMAND_QUEUE_GENERIC;

				Vulkan::ImageStagingCopyInfo copy{};
				copy.buffer_offset = 0;
				copy.image_offset = { 0, 0, 0 };
				copy.image_extent.width = width;
				copy.image_extent.height = height;
				copy.image_extent.depth = 1;
				copy.mip_level = 0;
				copy.base_array_layer = 0;
				copy.num_layers = 1;

				diffuse = device.CreateImage(diffuse_create_info,  pixels.size(), pixels.data(), 1, &copy);

				if (!diffuse)
					std::cout << "Failed to create image\n";

				Vulkan::ImageViewCreateInfo view_info{};
				view_info.image = diffuse;
				view_info.base_layer = 0;
				view_info.base_level = 1;
				view_info.view_type = VK_IMAGE_VIEW_TYPE_2D;
				
				diffuse_view = device.CreateImageView(view_info);

			}

			std::cout << "Diffuse texture loaded\n";

			glm::mat4 proj_matrix;
			glm::mat4 view_matrix;

			glm::vec4 light_position;
			glm::vec4 light_color;

			float shine;
			float reflectivity;
			float ambient;

			float theta = 0;
			float phi = 0;

			float radius = 0.6f;

			double last_time = 0;
			double current_delta = 0;

			double last_mouse_x = 0, last_mouse_y = 0;
			double mouse_dx = 0, mouse_dy = 0;
			
			while (platform.Alive(wsi))
			{
				if (is_mouse_pressed)
				{
					theta += mouse_dx * current_delta * 4.0f;
					phi += mouse_dy * current_delta * 4.0f;

					if (phi > 80.0f)
					{
						phi = 80.0f;
					}
					else if (phi < -80.0f)
					{
						phi = -80.0f;
					}
				}

				float camera_x = glm::cos(glm::radians(theta)) * radius * glm::cos(glm::radians(phi));
				float camera_y = glm::sin(glm::radians(theta)) * radius * glm::cos(glm::radians(phi));
				float camera_z = glm::sin(glm::radians(phi)) * radius;

				proj_matrix = glm::perspective(glm::radians(70.0f), (float)device.GetSwapchainWidth() / (float)device.GetSwapchainHeight(), .01f, 1000.0f);
				proj_matrix[1][1] *= -1;

				view_matrix = glm::lookAt(glm::vec3(camera_x, camera_y, camera_z), glm::vec3(0.0f, 0.25f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

				light_position = { 0.0f, 10.0f, 10.0f, 0.0f };
				light_color = { 1.0f, 1.0f, 1.0f, 0.0f };

				shine = 1;
				reflectivity = 0;
				ambient = .2f;

				Util::Timer timer;
				timer.start();

				wsi.BeginFrame();
				{	
					// Rendering process
					
					auto cmd = device.RequestCommandBuffer(Vulkan::CommandBuffer::Type::Generic);

					Vulkan::RenderPassInfo rp{};
					rp.num_color_attachments = 1;
					rp.color_attachments[0].view = &device.GetSwapchainView();
					rp.color_attachments[0].clear_color.float32[0] = 0.1f;
					rp.color_attachments[0].clear_color.float32[1] = 0.2f;
					rp.color_attachments[0].clear_color.float32[2] = 0.3f;

					rp.clear_attachments = ~0u;
					rp.store_attachments = 1u << 0;

					rp.depth_stencil.view = &device.GetPhysicalAttachment(device.GetSwapchainWidth(), device.GetSwapchainHeight(), device.GetDefaultDepthFormat());
					rp.depth_stencil.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;

					rp.op_flags |= Vulkan::RENDER_PASS_OP_CLEAR_DEPTH_STENCIL_BIT;

					Vulkan::RenderPassInfo::Subpass subpass{};
					subpass.num_color_attachments = 1;
					subpass.color_attachments[0] = 0;
					subpass.depth_stencil_mode = Vulkan::RenderPassInfo::DepthStencil::ReadWrite;

					rp.num_subpasses = 1;
					rp.subpasses = &subpass;

					cmd->BeginRenderPass(rp);

					cmd->SetProgram(*program);

					cmd->SetOpaqueState();
					cmd->SetDepthTest(true, true);
					cmd->SetDepthCompare(VK_COMPARE_OP_LESS);

					cmd->SetVertexAttrib(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
					cmd->SetVertexAttrib(1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3);
					cmd->SetVertexAttrib(2, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3 + sizeof(float) * 2);

					cmd->SetVertexBinding(0, sizeof(float) * 8);
					
					cmd->SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
					cmd->SetCullMode(VK_CULL_MODE_NONE);

					cmd->BindVertexBuffer(0, *vertex_buffer, 0);
					cmd->BindIndexBuffer(*index_buffer, 0, VK_INDEX_TYPE_UINT32);

					uint8_t* vert_ubo = static_cast<uint8_t*>(cmd->AllocateConstantData(0, 0, 0, 144));
					*(glm::mat4*)vert_ubo = proj_matrix;
					vert_ubo += sizeof(glm::mat4);
					*(glm::mat4*)vert_ubo = view_matrix;
					vert_ubo += sizeof(glm::mat4);
					*(glm::vec4*)vert_ubo = light_position;
					vert_ubo += sizeof(glm::vec4);

					uint8_t* frag_ubo = static_cast<uint8_t*>(cmd->AllocateConstantData(0, 1, 0, 32));
					*(glm::vec4*)frag_ubo = light_color;
					frag_ubo += sizeof(glm::vec4);
					*(float*)frag_ubo = shine;
					frag_ubo += sizeof(float);
					*(float*)frag_ubo = reflectivity;
					frag_ubo += sizeof(float);
					*(float*)frag_ubo = ambient;
					frag_ubo += sizeof(float);
					*(float*)frag_ubo = 0.0f;
					frag_ubo += sizeof(float);

					cmd->SetSampledTexture(1, 0, 0, *diffuse_view, Vulkan::StockSampler::LinearWrap);

					cmd->DrawIndexed(index_count);

					cmd->EndRenderPass();
					device.Submit(cmd);
					
					// -----------------
				}

				wsi.EndFrame();

				double time = glfwGetTime();
				current_delta = time - last_time;
				last_time = time;

				mouse_dx = mouse_x - last_mouse_x;
				mouse_dy = mouse_y - last_mouse_y;

				last_mouse_x = mouse_x;
				last_mouse_y = mouse_y;

			}

			program.Reset();
			device.WaitIdle();
		}


		QM_LOG_TRACE("Detroying WSI\n");
	}

	glfwTerminate();
}