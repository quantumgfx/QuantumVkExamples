#include <quantumvk/quantumvk.hpp>

#include <iostream>
#include <thread>

#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS

#include <glm/gtc/matrix_transform.hpp>

#include "../common/glfw_platform.hpp"
#include "../common/file_loader.hpp"

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

				vertex_buffer = device.CreateBuffer(vert_create_info, Vulkan::RESOURCE_EXCLUSIVE_GENERIC, vertices.data());

				Vulkan::BufferCreateInfo index_create_info{};
				index_create_info.domain = Vulkan::BufferDomain::Device;
				index_create_info.size = sizeof(uint32_t) * indices.size();
				index_create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

				index_buffer = device.CreateBuffer(index_create_info, Vulkan::RESOURCE_EXCLUSIVE_GENERIC, indices.data());
			}

			std::cout << "Loading diffuse texture\n";

			Vulkan::ImageHandle diffuse;

			{
				int width, height;
				std::vector<unsigned char> pixels = LoadTexture(diffuse_file, width, height);

				std::cout << "Texture has width: " << width << " and height " << height << "\n";

				Vulkan::ImageCreateInfo diffuse_create_info = Vulkan::ImageCreateInfo::Immutable2dImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, true);

				Vulkan::ImageStagingCopyInfo copy{};
				copy.buffer_offset = 0;
				copy.image_offset = { 0, 0, 0 };
				copy.image_extent.width = width;
				copy.image_extent.height = height;
				copy.image_extent.depth = 1;
				copy.mip_level = 0;
				copy.base_array_layer = 0;
				copy.num_layers = 1;

				diffuse = device.CreateImage(diffuse_create_info, Vulkan::RESOURCE_EXCLUSIVE_GENERIC, pixels.size(), pixels.data(), 1, &copy);

				if (!diffuse)
					std::cout << "Failed to create image\n";
			}

			std::cout << "Diffuse texture loaded\n";

			glm::mat4 proj_matrix;
			glm::mat4 view_matrix;

			glm::vec4 light_position;
			glm::vec4 light_color;

			float shine;
			float reflectivity;
			float ambient;
			
			while (platform.Alive(wsi))
			{
				proj_matrix = glm::perspective(glm::radians(70.0f), (float)device.GetSwapchainWidth() / (float)device.GetSwapchainHeight(), .01f, 1000.0f);
				view_matrix = glm::lookAt(glm::vec3(0.1f, .5f, 0.5f), glm::vec3(0.0f, 0.25f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

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
					rp.color_attachments[0] = &device.GetSwapchainView();
					rp.clear_attachments = ~0u;

					rp.clear_color[0].float32[0] = 0.1f;
					rp.clear_color[0].float32[1] = 0.2f;
					rp.clear_color[0].float32[2] = 0.3f;

					rp.store_attachments = 1u << 0;

					rp.depth_stencil = &device.GetPhysicalAttachment(device.GetSwapchainWidth(), device.GetSwapchainHeight(), device.GetDefaultDepthFormat());

					Vulkan::RenderPassInfo::Subpass subpass{};
					subpass.num_color_attachments = 1;
					subpass.color_attachments[0] = 0;
					subpass.depth_stencil_mode = Vulkan::RenderPassInfo::DepthStencil::ReadWrite;

					rp.num_subpasses = 1;
					rp.subpasses = &subpass;

					cmd->BeginRenderPass(rp);

					cmd->SetProgram(program);

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

					cmd->SetSampledTexture(1, 0, 0, diffuse->GetView(), Vulkan::StockSampler::LinearWrap);

					cmd->DrawIndexed(index_count);

					cmd->EndRenderPass();
					device.Submit(cmd);
					
					// -----------------
				}

				wsi.EndFrame();
			}

			program.Reset();
			device.WaitIdle();
		}


		QM_LOG_TRACE("Detroying WSI\n");
	}

	glfwTerminate();
}