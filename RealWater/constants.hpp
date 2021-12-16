#pragma

// File path
static const char* PARTICLE_SHADER_VERTEX = "shaders/sh_v_particle.glsl";
static const char* PARTICLE_SHADER_FRAGMENT = "shaders/sh_f_particle.glsl";
static const char* COMPUTE_SHADER = "shaders/sh_compute.glsl";

// Particle sytem
const int INIT_DRAW_TYPE = 1;
const int UPDATE_DRAW_TYPE = 2;
const float RADIUS = 0.04f;
const int PARTICLE_NUM_BASE = 16; //24 16 8
// Boudning type
const int TYPE_X_AXIS = 0;
const int TYPE_Z_AXIS = 1;

