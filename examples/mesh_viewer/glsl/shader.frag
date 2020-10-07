#version 450

layout(location = 0) in vec2 frag_tex_coords;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec3 to_light_vector;
layout(location = 3) in vec3 to_camera_vector;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 1) uniform FRAG_UNIFORM_BUFFER 
{
	vec4 light_color;
	float shine;
	float reflectivity;
	float ambient;
	float word;
	
} ubo;

layout(set = 1, binding = 0) uniform sampler2D diffuse_texture;

void main()
{
	vec3 unit_normal = normalize(frag_normal);
	vec3 unit_camera_vector = normalize(to_camera_vector);
	vec3 unit_ligh_vector = normalize(to_light_vector);

	vec3 totalDiffuse = vec3(0.0);
	vec3 totalSpecular = vec3(0.0);
	
	float normal_dot_light = dot(unit_normal, unit_ligh_vector);
	float brightness = max(normal_dot_light,0.0);
	vec3 light_direction = -unit_ligh_vector;
	vec3 reflected_light_direction = reflect(light_direction, unit_normal);
	float specular_factor = dot(reflected_light_direction , unit_camera_vector);
	specular_factor = max(specular_factor,0.0);
	float damped_factor = pow(specular_factor, ubo.shine);
	
	vec3 diffuse = brightness * ubo.light_color.xyz;
	vec3 specular = damped_factor * ubo.reflectivity * ubo.light_color.xyz;
	
	//ambient lighting
	diffuse = max(diffuse, ubo.ambient);

	vec4 texture_color = texture(diffuse_texture, frag_tex_coords);

	out_color = vec4(diffuse, 1.0)*texture_color + vec4(specular, 1.0);
}