#include <iostream>

/* imgui */
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "utils.hpp"
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

/* Particle */
#include "ParticleManager.hpp";

using namespace std;
using namespace glm;


// Initialized functions
void initOpenGL();
void initGLFW();
void initState();
void initParticle();

// Callback functions
void display();
void idle();
void window_size(GLFWwindow* window, int width, int height);
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
void cleanup();

// Util functions
void getUniformLocations();
void configureUniforms();

GLFWwindow* window;
GLuint width;
GLuint height;

// Particle 
ParticleManager* particleManager;
GLuint particleShader;
GLuint computeShader;
static int N;
GLuint uniModel;
GLuint uniDeltaTime;
static bool isStart;
static bool isReset;
static float rotX = 0.f;
static float rotY = 0.f;
static float scaleRatio = 0.2f;
static float imguiDeltaTime = 0.00025;
static bool imguiTimeType = false;
static int imguiFPS;
static int imguiParticleGenMode = 4;
static int imguiShadingMode = 2;
static bool imguiSetLight = true;
static int imguiParticleNum = PARTICLE_NUM_BASE * PARTICLE_NUM_BASE * PARTICLE_NUM_BASE;
static float imguiBoundingZ = 3.2f;
static float imguiBoundingX = 3.2f;
static const char* imguiShadingModeItems[] = { "Default", "Velocity Visual", "Surface Color"};

// Camera Ddata
GLuint uniView;
GLuint uniProj;
float camOrigin[3] = { 0.f, 0.f, 3.f };
float lookAtPoint[3] = { 0.f, 0.f, 0.f };
float fov = 60.f;

// Uniform Location
GLint uniShadingMode;
GLint uniSetLight;

// Time
float prevTime;
float currTime;
float deltaTime;
float timeStep;
int frameCount;
float timer;


int main(int argc, char** argv)
{
    try {
        initState();
        initGLFW();
        initOpenGL();
        initParticle();
    }
    catch (const exception& e) {
        // Handle any errors
        cerr << "Fatal error: " << e.what() << endl;
        cleanup();
        return -1;
    }

    //Init imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    while (!glfwWindowShouldClose(window))
    {
        idle();
        display();
        glfwPollEvents();
    }

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}

void initState()
{
    width = 800;
    height = 800;
    window = NULL;

    // Particle
    N = PARTICLE_NUM_BASE;  // pow(x, 3) must be multiple of 256(group size)
    particleManager = NULL;
    particleShader = 0;
    computeShader = 0;
    uniModel = 0;
    uniDeltaTime = 0;
    isStart = false;
    isReset = true;

    // Camera 
    uniView = 0;
    uniProj = 0;

    // Uniform Location
    uniShadingMode = 0;

    // Time
    prevTime = 0.f;
    currTime = 0.f;
    deltaTime = 0.f;
    frameCount = 0;
    timer = 0.f;

    // Force images to load vertically flipped
    // OpenGL expects pixel data to start at the lower-left corner
    stbi_set_flip_vertically_on_load(1);
}

void initParticle()
{
    // Particle
    // disp particle shader
    vector<GLuint> particleShaders;
    particleShaders.push_back(compileShader(GL_VERTEX_SHADER, PARTICLE_SHADER_VERTEX));
    particleShaders.push_back(compileShader(GL_FRAGMENT_SHADER, PARTICLE_SHADER_FRAGMENT));
    particleShader = linkProgram(particleShaders);
    for (auto s = particleShaders.begin(); s != particleShaders.end(); ++s)
        glDeleteShader(*s);
    // Get all uniform locations
    getUniformLocations();
    particleShaders.clear();

    // Compute shader
    GLuint computeShader0;
    computeShader0 = compileShader(GL_COMPUTE_SHADER, COMPUTE_SHADER);
    computeShader = glCreateProgram();
    glAttachShader(computeShader, computeShader0);
    glLinkProgram(computeShader);
    glDeleteShader(computeShader0);
    glUseProgram(0);

    assert(glGetError() == GL_NO_ERROR);

}

void initGLFW()
{
    glfwInit();
    if (!glfwInit())
    {
        string err = "failed to init glfw!";
        throw(runtime_error(err));
    }

    window = glfwCreateWindow(width, height, "Real Water", NULL, NULL);
    if (!window)
    {
        string err = "failed to create window!";
        glfwTerminate();
        throw(runtime_error(err));
    }

    // Register callback functions with glfw.
    glfwSetKeyCallback(window, keyboard);
    glfwSetWindowSizeCallback(window, window_size);

    glfwMakeContextCurrent(window);
}

void getUniformLocations()
{
    uniProj = glGetUniformLocation(particleShader, "P");
    if (uniProj == -1)
        cout << "P uniform getting failed!" << endl;

    uniView = glGetUniformLocation(particleShader, "V");
    if (uniView == -1)
        cout << "V uniform getting faield!" << endl;

    uniModel = glGetUniformLocation(particleShader, "M");
    if (uniModel)
        cout << "M uniform getting failed!" << endl;

    uniShadingMode = glGetUniformLocation(particleShader, "shading_mode");
    if (uniShadingMode == -1)
        cout << "shading_mode uniform getting failed!" << endl;

    uniSetLight = glGetUniformLocation(particleShader, "set_light");
    if (uniSetLight == -1)
        cout << "set light uniform getting failed!" << endl;
}

void initOpenGL()
{
    glewInit();
    
    // Basic setup
    glClearColor(1.f, 0.992f, 0.894f, 1.f);
    glClearDepth(1.f);
    glEnable(GL_DEPTH_TEST);
    // Enable use of gl_PointSize in vertex shader
    glEnable(GL_PROGRAM_POINT_SIZE);
    // Enable use of gl_PointCoord in fragment shader
    glEnable(GL_POINT_SPRITE);
    // Enable Alpha Blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    assert(glGetError() == GL_NO_ERROR);
}

void draw_gui(GLFWwindow* window)
{
    // Begin imgui frame.
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Camera Contorl window
    ImGui::Begin("Camera Contorl");
        ImGui::SetWindowFontScale(1.f);
        ImGui::SliderFloat3("Origin", camOrigin, -5.f, 5.f);
        ImGui::SliderFloat3("Look Direction", lookAtPoint, -1.f, 1.f);
        ImGui::SliderFloat("Field of View", &fov, 10.f, 100.f);
    ImGui::End();

    // Particle Contorl window
    ImGui::Begin("Paricle Control");
        ImGui::SetWindowFontScale(1.f);
        ImGui::Text("Action Contorl");
        if (ImGui::Button("Start"))
        {
            isStart = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Pause"))
        {
            isStart = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset"))
        {
            isStart = false;
            isReset = true;
        }

        ImGui::Text("Particle System Model Control");
        ImGui::SliderFloat("Scale", &scaleRatio, 0.05f, 2.f);
        ImGui::SliderFloat("Rotate X", &rotX, 0.f, 180.f);
        ImGui::SliderFloat("Rotate Y", &rotY, 0.f, 180.f);

        ImGui::Text("Particle Generate Mode");
        ImGui::RadioButton("Random Cube Mode", &imguiParticleGenMode, 0);
        ImGui::SameLine();
        ImGui::RadioButton("DropDown Mode", &imguiParticleGenMode, 1);
        ImGui::RadioButton("Sorted Plane 1", &imguiParticleGenMode, 2);
        ImGui::SameLine();
        ImGui::RadioButton("Sorted Plane 2", &imguiParticleGenMode, 3);
        ImGui::RadioButton("Sorted Cube Mode", &imguiParticleGenMode, 4);

        ImGui::Text("Particle Bounding Setting");
        ImGui::SliderFloat("X", &imguiBoundingX, 1.f, 7.f);
        ImGui::SliderFloat("Z", &imguiBoundingZ, 1.f, 4.f);

        ImGui::Text("Particle Shading Mode");
        ImGui::Combo("Shading Mode", &imguiShadingMode, imguiShadingModeItems, IM_ARRAYSIZE(imguiShadingModeItems));
        
        ImGui::Text("Particle Lighting (only works in 'surface color' mode)");
        ImGui::Checkbox("", &imguiSetLight);

        ImGui::Text("Only if check this box can change time step!");
        ImGui::Checkbox("Set Time Stamp", &imguiTimeType);
        ImGui::SameLine();
        ImGui::SliderFloat("", &imguiDeltaTime, 1.f, 50.f);

        
    ImGui::End();

    ImGui::Begin("Parameters Information Panel");
        ImGui::Text("Particle Num: %d", imguiParticleNum);
        ImGui::SameLine();
        ImGui::Text("FPS: %d", imguiFPS);
        ImGui::Text("Delta Time %.3f ms", deltaTime * 1000);
    ImGui::End();

    /*static bool show_demo = true;
    ImGui::ShowDemoWindow(&show_demo);*/

    // End imgui frame.
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void configureUniforms()
{
    // Make sure call glUseProgram before call this function
    glUniform1i(uniShadingMode, imguiShadingMode);
    glUniform1i(uniSetLight, imguiSetLight);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Render Particles
    if (isReset) {
        particleManager = new ParticleManager(N*N*N, imguiParticleGenMode, particleShader, computeShader);
        isReset = false;
    }

    glUseProgram(particleShader);

    // Set up Camera
    float winAspect = (float)width / (float)height;
    // Create perspective projection matrix
    mat4 proj = perspective(fov, winAspect, 0.1f, 100.0f);
    // Create view transformation matrix
    vec3 eyePos = vec3(camOrigin[0], camOrigin[1], camOrigin[2]);
    mat4 view = lookAt(
        eyePos,
        vec3(lookAtPoint[0], lookAtPoint[1], lookAtPoint[2]),
        vec3(0.0f, 1.0f, 0.0f)
    );
    glUniformMatrix4fv(uniView, 1, GL_FALSE, value_ptr(view));
    glUniformMatrix4fv(uniProj, 1, GL_FALSE, value_ptr(proj));

    // Create model matrix
    mat4 rot = rotate(mat4(1.0f), rotX, vec3(1.0, 0.0, 0.0));
    rot = rotate(rot, rotY, vec3(0.0, 1.0, 0.0));
    mat4 scal = scale(mat4(1.f), vec3(scaleRatio));
    mat4 model = scal * rot;
    glUniformMatrix4fv(uniModel, 1, GL_FALSE, value_ptr(model));

    // Configure other uniform attributes
    configureUniforms();
    
    if (isStart == true)
    {
        particleManager->setBounding(TYPE_X_AXIS, imguiBoundingX);
        particleManager->setBounding(TYPE_Z_AXIS, imguiBoundingZ);
        particleManager->draw(timeStep, UPDATE_DRAW_TYPE);
    }
    else
    {
        particleManager->draw(timeStep, INIT_DRAW_TYPE);
    }

    glUseProgram(0);

    // Draw ImGui
    draw_gui(window);

    glfwSwapBuffers(window);
}

void idle()
{
    currTime = static_cast<float>(glfwGetTime());
    deltaTime = currTime - prevTime;
    prevTime = currTime;

    // Calculate fps
    timer += deltaTime;
    frameCount++;
    if (timer > 1.f)
    {
        imguiFPS = frameCount;
        timer = 0.f;
        frameCount = 0;
    }

    // Set calculate time type
    if (imguiTimeType)
        timeStep = imguiDeltaTime / 1000.f;
    else
        timeStep = deltaTime;
}

void window_size(GLFWwindow* window, int width, int height)
{
    ::width = width;
    ::height = height;
    glViewport(0, 0, width, height);
}

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        }
    }
}

void cleanup()
{
    if (window) window = NULL;
    if (computeShader) { glDeleteProgram(computeShader); computeShader = 0; }
    if (particleShader) { glDeleteProgram(particleShader); particleShader = 0; }
    
    // clear uniform location
    uniModel = 0;
    uniProj = 0;
    uniView = 0;
    uniDeltaTime = 0;
    uniShadingMode = 0;
    uniSetLight = 0;
}


