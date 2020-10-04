#version 450

layout(location = 0) in vec2 in_pos;

layout(location = 0) out vec2 frag_pos;

void main()
{

	gl_Position = vec4(in_pos, 0.0, 1.0);
	frag_pos = in_pos;

}