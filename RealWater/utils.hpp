#ifndef _UTILS_HPP
#define _UTILS_HPP

#include <string>
#include <vector>
#include <GL/glew.h>

using namespace std;

GLuint compileShader(GLenum type, string filename, string prepend = "");
GLuint linkProgram(vector<GLuint> shaders);

#endif // !_UTILS_HPP

