#version 460 compatibility
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

struct particle
{
	vec4 prevPos;
	vec4 currPos;
	vec4 vel;
	vec4 acc;
	vec4 surfaceNorm;
	vec4 factor; // 0=density, 1=pressure 2=color field
};

layout(std430, binding = 0) buffer Particle
{
	particle particles[];
};

const float RADIUS = 0.04f;
const float CORE_RAIDUS = RADIUS * 10;
const float MASS = 80.0f;
const float REST_DENSITY = 100.f;
const float STIFFNESS = 10.f;
const float VISCOSITY = 200.f;
const vec3 GRAVITY = vec3(0.f, -10.f, 0.f);
const float PI = 3.1415926535f;
const float SPEED_DECAY = 0.8;
const float SURFACE_TENSION = 10.f;

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;  // group size

uniform int N;					// number of particls
uniform float delta_time;		// delta time each frame
uniform int pass;
uniform float bounding_z;		// set the bounding range for z
uniform float bounding_x;		// set the bounding range for x


void main()
{
	uint i = gl_GlobalInvocationID.x;  // 1D, so the .y and .z is both 1

	if (pass == 1)
	{
		// Density and Pressure
		float nb_sum = 0.f;
		for (int j = 0; j < N; j++)
		{
			float dist = distance(particles[i].currPos, particles[j].currPos);
			if (dist < CORE_RAIDUS)
			{
				nb_sum += pow(pow(CORE_RAIDUS, 2) - pow(dist, 2), 3);
			}
		}
		
		// Density
		float density_i = MASS * 315 / (64 * PI * pow(CORE_RAIDUS, 9)) * nb_sum;
		// Pressure
		float pressure_i = max(STIFFNESS * (density_i - REST_DENSITY), 0.f);

		// Update into the particle i factor 
		particles[i].factor.x = density_i;  // density 
		particles[i].factor.y = pressure_i; // pressure
		
	} 
	
	else if (pass == 2)
	{
		// Calculate acc in pressure, viscosity, gravity
		vec3 nb_pacc_sum = vec3(0.f);			// pressure acc sum 
		vec3 nb_vacc_sum = vec3(0.f);			// viscosity acc sum
		vec3 nb_sacc_sum = vec3(0.f);			// surface tension sum
		float nb_color_surface_sum = 0.f;		// color field sum
		vec3 nb_surface_normal_sum = vec3(0.f); // surface nromal sum

		for (int j = 0; j < N; j++)
		{
			float dist = distance(particles[i].currPos, particles[j].currPos);
			if (dist < CORE_RAIDUS && i != j)
			{
				// sum up quantity in pressure direction related to neighbour
				float pressure_ij = particles[i].factor.y + particles[j].factor.y;
				float density_ij = particles[i].factor.x * particles[j].factor.x;
				float r_diff_pow_2 = pow(CORE_RAIDUS - dist, 2); 
				vec3 dir_ij = particles[i].currPos.xyz - particles[j].currPos.xyz;	
				nb_pacc_sum += normalize(dir_ij) * (pressure_ij / (2.f * density_ij)) * r_diff_pow_2;
			
				// sum up quantity in viscosity direction related to neighbour
				vec3 velocity_ji = particles[j].vel.xyz - particles[i].vel.xyz;
				nb_vacc_sum += velocity_ji / density_ij * (CORE_RAIDUS - dist);

				// sum up quantity in color field
				nb_color_surface_sum += (1.f / particles[j].factor.x) * pow(pow(CORE_RAIDUS, 2) - pow(dist, 2), 3);

				// sum up surface normal 
				nb_surface_normal_sum += (1.f / particles[j].factor.x) * pow(pow(CORE_RAIDUS, 2) - pow(dist, 2), 2) * dir_ij;

				// sum up surface tension
				nb_sacc_sum += (1.f / density_ij) * (pow(CORE_RAIDUS, 2) - pow(dist, 2)) * (pow(dist, 2) - 3.f/4.f * (pow(CORE_RAIDUS,2) - pow(dist,2)));
			}
			
		}

		// write color field to buffer
		float color_field = MASS * 315.f / (64.f * PI * pow(CORE_RAIDUS, 9)) * nb_color_surface_sum;
		particles[i].factor.z = color_field;
//		if (abs(color_field - 0.f) < 0.01f )
//			particles[i].factor.z = 0;
//		else 
//			particles[i].factor.z = 1;

		// write surface normal to buffer (should normalize)
		vec3 surface_normal = -MASS * 945.f / (32.f * PI * pow(CORE_RAIDUS, 9)) * nb_surface_normal_sum;
		particles[i].surfaceNorm = vec4(normalize(surface_normal), 0.f);

		// acc in pressure
		vec3 acc_pressure_i = MASS * 45.f / (PI * pow(CORE_RAIDUS, 6)) * nb_pacc_sum;
		// acc in viscosity
		vec3 acc_viscosity_i =  MASS * VISCOSITY * 45.f / (PI * pow(CORE_RAIDUS, 6)) * nb_vacc_sum;
		// acc in gravity
		vec3 acc_gravity_i = GRAVITY;
		// acc in surface tension
		vec3 acc_surface_tension = -MASS * SURFACE_TENSION * 945.f / (8.f * PI * pow(CORE_RAIDUS, 9)) *
			(nb_sacc_sum * particles[i].surfaceNorm.xyz);

		// write acc to the buffer
		vec3 acc = acc_pressure_i + acc_viscosity_i + acc_gravity_i;
		particles[i].acc = vec4(acc, 1.f);
		vec3 vel = particles[i].vel.xyz + acc * delta_time;
		particles[i].vel = vec4(vel, 1.f);

	}

	else if (pass == 3)
	{
		vec4 currPos = particles[i].currPos + particles[i].vel * delta_time;
		vec4 prevPos = particles[i].currPos;
		vec4 vel = particles[i].vel;

		// detect bouding to correct the position
		if (currPos.x < -bounding_x)
		{
			currPos.x = -bounding_x;
			vel.x = -vel.x * SPEED_DECAY;
		}
		else if (currPos.x > bounding_x)
		{
			currPos.x = bounding_x;
			vel.x = -vel.x * SPEED_DECAY;
		}
		if (currPos.y < -6.f)
		{
			currPos.y = -6.f;
			vel.y = -vel.y * SPEED_DECAY;
		}
//		else if (currPos.y > 4.f)
//		{
//			currPos.y = 4.f;
//			vel.y = -vel.y * SPEED_DECAY;
//		}
		if (currPos.z < -bounding_z)
		{
			currPos.z = -bounding_z;
			vel.z = -vel.z * SPEED_DECAY;
		}
		else if (currPos.z > bounding_z)
		{
			currPos.z = bounding_z;
			vel.z = -vel.z * SPEED_DECAY;
		}

		particles[i].currPos = currPos;
		particles[i].prevPos = prevPos;
		particles[i].vel = vel;
	}
	
}