#pragma once
#include "Window.h"
#include "Camera.h"
#include "Shader.h"
#include "Mesh.h"
#include "FBXLoader.h"
#include "AnimController.h"
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <opencv2/opencv.hpp>

class Render {
public:
    Render(Window& window,
        Camera& camera,
        Shader& shader,
        Mesh& mesh,
        FBXLoader& fbxLoader,
        AnimController& animController,
        const std::string& diffuseTexturePath);

    bool InitializeRender();

    // Run the render loop 
    void run();

private:
    Window& window_;
    Camera& camera_;
    Shader& shader_;
    Mesh& mesh_;
    FBXLoader& loader_;
    AnimController& animController_;
    std::string texPath_;

    GLuint diffuseTex_ = 0;
    glm::mat4 proj_;

    double animTimeAcc_ = 0.0;
    double lastTime_ = 0.0;

    // GLFW callbacks must be static and non-capturing
    static void mouseButtonCallback(GLFWwindow* w, int b, int a, int m);
    static void cursorPosCallback(GLFWwindow* w, double x, double y);
    static void scrollCallback(GLFWwindow* w, double xoff, double yoff);
    static void keyCallback(GLFWwindow* w, int key, int sc, int action, int mods);
};




