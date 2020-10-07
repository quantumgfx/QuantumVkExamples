#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_tex_coords;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec2 frag_tex_coords;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec3 to_light_vector;
layout(location = 3) out vec3 to_camera_vector;

layout(set = 0, binding = 0) uniform VERT_UNIFORM_BUFFER 
{
	mat4 proj;
	mat4 view;
	vec4 light_pos;
} ubo;

void main()
{
	vec4 world_position = vec4(in_pos, 1.0);
	
	gl_Position = ubo.proj*ubo.view*world_position;
	
	frag_tex_coords = in_tex_coords;
	frag_normal = in_normal;
	
	to_light_vector = ubo.light_pos.xyz - world_position.xyz;
	to_camera_vector = (inverse(ubo.view) * vec4(0.0,0.0,0.0,1.0)).xyz - world_position.xyz;
}