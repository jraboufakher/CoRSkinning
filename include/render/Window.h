#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>

/// Encapsulates GLFW window creation, context init, and basic event/swap handling.
class Window {
public:
    Window(int width, int height, const char* title);
    ~Window();

    // Initialize GLFW, create window, init GLEW, set up viewport and basic GL state.
    bool InitializeWindow();

    // Whether the user has requested to close the window.
    bool shouldClose() const;

    // Poll for and process pending events.
    void pollEvents() const;

    // Swap front and back buffers.
    void swapBuffers() const;

    // Get raw GLFWwindow* for setting callbacks.
    GLFWwindow* get() const;

private:
    GLFWwindow* window_;
    int         width_;
    int         height_;
    const char* title_;
};