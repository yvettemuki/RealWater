#version 460

uniform int shading_mode;

// Lighting 
uniform bool set_light;
uniform float alpha;
vec3 light_pos = vec3(1.0f, 0.f, 0.0f);
uniform vec3 eye_pos;
vec3 La = vec3(0.8f);
vec3 Ld = vec3(0.6f);

// In 
in vec3 velocity;
in float color_field;
in vec3 surface_normal_w;
in vec3 pos_w;

// Constant
vec4 constant_color = vec4(0.160, 0.501, 0.725, 1.f);

// Out
vec4 out_color;
out vec4 fragcolor;

void phongLighting()
{
	// ambient
	vec4 ambient_color = vec4(vec3(out_color) * La, 1.f);

	// diffuse 
	vec3 light_dir = normalize(light_pos - pos_w);
	float diff_cos_theta = max(dot(-normalize(surface_normal_w), light_dir), 0.f);
	float gamma = 1.f;
	vec4 diffuse_color = vec4(pow(vec3(out_color) * Ld * diff_cos_theta, vec3(1.f / gamma)), 1.f);
	out_color = ambient_color + diffuse_color;
}

void main()
{
	// Shape the point(quad) with discard
	const float r = distance(vec2(0.5f), gl_PointCoord.xy);
	if (r > 0.5)
	{
		discard;
	}

	// Cenerate Mixing color
	float mix_value = distance(gl_PointCoord.xy, vec2(0.5f, 0.f));
	vec4 shading_color = vec4(mix(vec3(0.525, 0.658, 0.905), vec3(0.568, 0.917, 0.894), mix_value), 1.f);

	// Generate color by velocity
	vec4 velocity_intensity = vec4(length(velocity), 0.f, 0.f, 1.f);

	switch (shading_mode)
	{
	// default mode
	case 0:
		out_color = shading_color;
		break;
	// velocity mode
	case 1:
		out_color = shading_color + velocity_intensity;
		// calculate depth
		//float depth = gl_FragCoord.z;
		//float z = depth * 2.f - 1.f;	// NDC space z
		//float near = 0.1f;
		//float far = 100.f;
		//depth = (2.f * near * far) / (far + near - z * (far - near));
		//out_color = vec4(vec3( 10 * depth / far), 1.f);
		break;

	// surface color_field
	case 2:
	{
		// shading with color field
		out_color = constant_color + 0.2 * (1 - color_field);
		// set lighting
		if (set_light)
			phongLighting();
		break;
	}	
	}

	fragcolor = vec4(vec3(out_color), 1.f);
}

