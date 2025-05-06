#include "render/Window.h"
#include <iostream>

Window::Window(int width, int height, const char* title)
    : width_(width), height_(height), title_(title), window_(nullptr)
{}

Window::~Window()
{
    if (window_) {
        glfwDestroyWindow(window_);
        glfwTerminate();
    }
}

bool Window::InitializeWindow()
{
    if (!glfwInit()) {
        std::cerr << "GLFW init failed." << std::endl;
        return false;
    }

    window_ = glfwCreateWindow(width_, height_, title_, nullptr, nullptr);
    if (!window_) {
        std::cerr << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW init failed" << std::endl;
        return false;
    }

    // Set viewport to cover the whole window
    glViewport(0, 0, width_, height_);

    // Basic GL state
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    return true;
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(window_) != 0;
}

void Window::pollEvents() const {
    glfwPollEvents();
}

void Window::swapBuffers() const {
    glfwSwapBuffers(window_);
}

GLFWwindow* Window::get() const {
    return window_;
}
