#include "ParticleManager.hpp"
#include "glm/glm.hpp";
#include <random>
#include <cstdlib>

mt19937 rng;
uniform_real_distribution<float> noise;
// Random number generator for float
float random(float min, float max)
{
	random_device rd;
	rng = mt19937(rd());
	noise = uniform_real_distribution<float>(min, max);
	return noise(rng);
}

ParticleManager::ParticleManager(unsigned int particleNum, int mode, GLuint shader, GLuint computeShader) : 
	particleNum(particleNum), 
	mode(mode),
	shader(shader), 
	computeShader(computeShader)
{
	init(mode);
}

void ParticleManager::init(int particleGenMode)
{
	// Particles
	// Initialize particle data
	vector<Particle> particles;

	switch (particleGenMode)
	{
	case 0:
	{
		//Particle cube with random generate particle
		for (int i = 0; i < particleNum; i++)
		{
			vec4 initPos = vec4(random(-2.f, -1.f), random(0.5f, 1.f), random(0.25f, 0.5f), 1.f);
			Particle particle;
			particle.currPos = initPos;
			particle.prevPos = initPos;
			particle.vel = vec4(0.f);
			particle.acc = vec4(0.f);
			particle.surfaceNorm = vec4(0.f);
			particle.factor = vec4(vec3(0.f), 1.f);
			particles.push_back(particle);
		}
		break;
	}
		
	case 1:
	{
		// Particle cube (with d)
		float offset = -(RADIUS * 2 * PARTICLE_NUM_BASE / 2) + RADIUS;
		float d = 2 * RADIUS;
		for (int i = 0; i < PARTICLE_NUM_BASE; i++)
		{
			for (int j = 0; j < PARTICLE_NUM_BASE; j++)
			{
				for (int k = 0; k < PARTICLE_NUM_BASE; k++)
				{
					vec4 initPos = vec4(offset + d * i, offset + d * j, offset + d * k, 1.f);
					Particle particle;
					particle.currPos = initPos;
					particle.prevPos = initPos;
					particle.vel = vec4(0.f);
					particle.acc = vec4(0.f);
					particle.surfaceNorm = vec4(0.f);
					particle.factor = vec4(0.f);
					particles.push_back(particle);
				}
			}
		}
		break;
	}
		

	case 2:
	{
		// Sorted plane 1 (with d)
		int range = glm::sqrt((float)particleNum) / 2.f;
		float d = RADIUS * 2;
		float offset = -range * d + RADIUS;
		float offsetY = 0.02f;
		for (int i = 0; i < range * 2; i++)
		{
			for (int j = 0; j < range * 2; j++)
			{
				float offsetZ = 0.04;
				if (j % 2)
					offsetZ = -offsetZ;
				vec4 initPos = vec4(offset + d * i, offset + d * j + offsetY, offsetZ, 1.f);
				//cout << initPos.x << ", " << initPos.y << ", " << initPos.z << endl;
				Particle particle;
				particle.currPos = initPos;
				particle.prevPos = initPos;
				particle.vel = vec4(0.f);
				particle.acc = vec4(0.f);
				particle.surfaceNorm = vec4(0.f);
				particle.factor = vec4(vec3(0.f), 1.f);
				particles.push_back(particle);
			}
		}
		break;
	}

	case 3:
	{
		// Sorted plane 2 (with 2 * d)
		int range = glm::sqrt((float)particleNum) / 2.f;
		float d = RADIUS * 2;
		float offset = -range * 2 * d + d;
		float offsetY = 0.5f;
		for (int i = 0; i < range * 2; i++)
		{
			for (int j = 0; j < range * 2; j++)
			{
				vec4 initPos = vec4(offset + 2 * d * i, offset + 2 * d * j, 0.f, 1.f);
				//cout << initPos.x << ", " << initPos.y << ", " << initPos.z << endl;
				Particle particle;
				particle.currPos = initPos;
				particle.prevPos = initPos;
				particle.vel = vec4(0.f);
				particle.acc = vec4(0.f);
				particle.surfaceNorm = vec4(0.f);
				particle.factor = vec4(vec3(0.f), 1.f);
				particles.push_back(particle);
			}
		}
		break;
	}

	case 4:
	{
		// Particle Cube with 4 * d
		float d = 2 * RADIUS;
		int range = PARTICLE_NUM_BASE / 2;
		float offset = -range * 4 * d + d;
		for (int i = 0; i < PARTICLE_NUM_BASE; i++)
		{
			for (int j = 0; j < PARTICLE_NUM_BASE; j++)
			{
				for (int k = 0; k < PARTICLE_NUM_BASE; k++)
				{
					vec4 initPos = vec4(offset + 4 * d * i, offset + 4 * d * j, offset + 4 * d * k, 1.f);
					Particle particle;
					particle.currPos = initPos;
					particle.prevPos = initPos;
					particle.vel = vec4(0.f);
					particle.acc = vec4(0.f);
					particle.surfaceNorm = vec4(0.f);
					particle.factor = vec4(vec3(0.f), 1.f);
					particles.push_back(particle);
				}
			}
		}

		break;
	}
		
	default:
		break;
	}

	// Generate SSBO
	glGenBuffers(1, &particleSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, particleNum * sizeof(Particle), particles.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
	//delete particles;
	particles.clear();

	// Bind Vertex Array Object
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
	glBindBuffer(GL_ARRAY_BUFFER, particleSSBO);
	// Position (currPos)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)(sizeof(vec4)));
	// Velocity
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)(2 * sizeof(vec4)));
	// Surface normal
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)(4 * sizeof(vec4)));
	// Factor (Density, Pressure, Color Field)
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)(5 * sizeof(vec4)));
	
	glBindVertexArray(0);

	/*GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
	Particle* particles = (struct Particle*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, N * sizeof(Particle), bufMask);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);*/

	// Get uniform location
	glUseProgram(computeShader);
	uniDeltaTime = glGetUniformLocation(computeShader, "delta_time");
	uniParticleNum = glGetUniformLocation(computeShader, "N");
	uniPass = glGetUniformLocation(computeShader, "pass");
	uniBoundingX = glGetUniformLocation(computeShader, "bounding_x");
	uniBoundingZ = glGetUniformLocation(computeShader, "bounding_z");
	glUseProgram(0);

	assert(glGetError() == GL_NO_ERROR);

}

void ParticleManager::initDraw()
{
	if (!shader)
	{
		cout << "shader can not be empty!" << endl;
		return;
	}

	glBindVertexArray(VAO);
	glUseProgram(shader);
	glDrawArrays(GL_POINTS, 0, particleNum);
	glBindVertexArray(0);
}

void ParticleManager::update(float deltaTime)
{
	if (!shader)
	{
		cout << "shader can not be empty!" << endl;
		return;
	}

	glBindVertexArray(VAO);

	// pass 1
	glUseProgram(computeShader);
	glUniform1i(uniParticleNum, particleNum);
	glUniform1f(uniDeltaTime, deltaTime);
	glUniform1f(uniBoundingX, boundingX);
	glUniform1f(uniBoundingZ, boundingZ);
	int pass_loc = glGetUniformLocation(computeShader, "pass");
	glUniform1i(pass_loc, 1);
	glDispatchCompute(particleNum / WORK_GROUP_SIZE, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	/*Particle* particles = (Particle*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, N, GL_MAP_READ_BIT);
	cout << particles[0].factor.x << endl;
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);*/


	//pass 2
	glUseProgram(computeShader);
	glUniform1i(uniParticleNum, particleNum);
	glUniform1f(uniDeltaTime, deltaTime);
	glUniform1f(uniBoundingX, boundingX);
	glUniform1f(uniBoundingZ, boundingZ);
	pass_loc = glGetUniformLocation(computeShader, "pass");
	glUniform1i(pass_loc, 2);
	glDispatchCompute(particleNum / WORK_GROUP_SIZE, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

	//pass 3
	glUseProgram(computeShader);
	glUniform1i(uniParticleNum, particleNum);
	glUniform1f(uniDeltaTime, deltaTime);
	glUniform1f(uniBoundingX, boundingX);
	glUniform1f(uniBoundingZ, boundingZ);
	pass_loc = glGetUniformLocation(computeShader, "pass");
	glUniform1i(pass_loc, 3);
	glDispatchCompute(particleNum / WORK_GROUP_SIZE, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

	// draw the particle display shader
	glUseProgram(shader);
	glDrawArrays(GL_POINTS, 0, particleNum);

	glBindVertexArray(0);

	assert(glGetError() == GL_NO_ERROR);
}



void ParticleManager::draw(float deltaTime, int drawType)
{
	if (drawType == INIT_DRAW_TYPE)
	{
		initDraw();
	}

	if (drawType == UPDATE_DRAW_TYPE)
	{
		update(deltaTime);
	}
	
}

void ParticleManager::setBounding(int axisType, float boundingVal)
{
	if (axisType == TYPE_X_AXIS)
	{
		// Set x range bounding
		this->boundingX = boundingVal;
	}
	else if (axisType == TYPE_Z_AXIS)
	{
		// Set z range bounding
		this->boundingZ = boundingVal;
	}
	
}


void ParticleManager::cleanup()
{
	VAO = 0;
	VBO = 0;
	particleSSBO = 0;
	
	// clean up uniform variables
	uniDeltaTime = 0;
	uniParticleNum = 0;
	uniPass = 0;
	uniBoundingZ = 0;
	uniBoundingX = 0;
}

ParticleManager::~ParticleManager()
{
	cleanup();
}



