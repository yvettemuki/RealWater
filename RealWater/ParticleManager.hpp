#ifndef _PARTICLE_MANAGER_HPP
#define _PARTICLE_MANAGER_HPP

#define WORK_GROUP_SIZE 256

#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include "constants.hpp";

using namespace glm;
using namespace std;


struct Particle
{
	vec4 prevPos;
	vec4 currPos;
	vec4 vel;
	vec4 acc;
	vec4 surfaceNorm;
	vec4 factor;
};


class ParticleManager {
public:
	ParticleManager(unsigned int particleNum, int mode, GLuint shader, GLuint computeShader);
	~ParticleManager();
	void init(int mode);			// init particle buffer data
	void initDraw();				// draw the init particles
	void update(float deltaTime);	// update the particles
	void draw(float deltaTime, int drawType);
	void setBounding(int axisType, float boundingVal);
	void cleanup();

	int particleNum;	// Particle number base (use base to get particle init matirx)
	vector<vec3> positions;

private:
	int mode;
	float prev_time;
	float curr_time;
	float delta_time;
	float boundingZ;
	float boundingX;
	GLuint VAO;
	GLuint VBO;
	GLuint shader;

	// Uniform Location
	GLuint uniDeltaTime;
	GLuint uniParticleNum;
	GLuint uniPass;
	GLuint uniBoundingZ;
	GLuint uniBoundingX;

	//SSBO
	GLuint computeShader;
	GLuint particleSSBO;
};

#endif // !_PARTICLE_MANAGER_HPP