#version 460

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 vel;
layout(location = 2) in vec4 surface_normal;
layout(location = 3) in vec4 factor; // 1.density 2.pressure 3.color_field 4.init_type

// Camera
uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

out vec3 velocity;
out float color_field;
out vec3 surface_normal_w;
out vec3 pos_w;

void main()
{
	mat4 mvp = P * V * M;
	gl_Position = mvp * vec4(vec3(pos), 1.f);
	gl_PointSize = 25.f;

	velocity = vec3(vel);
	color_field = factor.z;
	surface_normal_w = vec3(M * surface_normal);
	pos_w = vec3(M * pos);
}