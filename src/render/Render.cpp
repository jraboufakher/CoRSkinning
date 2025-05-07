#include "render/Render.h"
#include <iostream>
#include <cmath>
#include <glm/gtx/string_cast.hpp>

Render::Render(Window& window,
    Camera& camera,
    Shader& shader,
    Mesh& mesh,
    FBXLoader& fbxLoader,
    AnimController& animController,
    const std::string& diffuseTexturePath)
    : window_(window), camera_(camera), shader_(shader), mesh_(mesh), loader_(fbxLoader), animController_(animController), texPath_(diffuseTexturePath)
{}

bool Render::InitializeRender() {
    // Initialize window (GLFW + GLEW + viewport + depth test)
    if (!window_.InitializeWindow()) return false;

    // Install callbacks: use 'this' as user pointer
    glfwSetWindowUserPointer(window_.get(), this);
    glfwSetMouseButtonCallback(window_.get(), mouseButtonCallback);
    glfwSetCursorPosCallback(window_.get(), cursorPosCallback);
    glfwSetScrollCallback(window_.get(), scrollCallback);
    glfwSetKeyCallback(window_.get(), keyCallback);

    // Load and upload diffuse texture
    cv::Mat bgr = cv::imread(texPath_, cv::IMREAD_COLOR);
    if (bgr.empty()) {
        std::cerr << "Could not open or find texture: " << texPath_ << std::endl;
        return false;
    }
    cv::Mat rgba;
    cv::cvtColor(bgr, rgba, cv::COLOR_BGR2RGBA);

    glGenTextures(1, &diffuseTex_);
    glBindTexture(GL_TEXTURE_2D, diffuseTex_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        rgba.cols, rgba.rows,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        rgba.data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Initialize mesh buffers (VBO/VAO/EBO)
    //mesh_.flattenVertices();
    mesh_.initBuffers();

    // Compile/link shader
    if (!shader_.LoadShaders("../shaders/skin.vert", "../shaders/skin.frag")) {
        std::cerr << "Failed to load skin shaders" << std::endl;
        return false;
    }

    proj_ = glm::perspective(
        glm::radians(45.0f),
        800.0f / 600.0f,
        0.1f,
        100.0f
    );

    // Initialize timer
    lastTime_ = glfwGetTime();

    // Initialize AnimController
    animController_.Initialize(loader_);
    return true;
}

void Render::run() {
    const auto& skel = loader_.GetSkeletonData();
    double startSec = animController_.getStartTime();
    double endSec = animController_.getEndTime();
    double duration = endSec - startSec;

    // Render loop
    while (!window_.shouldClose()) {
        window_.pollEvents();

        // Timing
        double now = glfwGetTime();
        double delta = now - lastTime_;
        lastTime_ = now;
        if (animController_.isPlaying()) {
            animTimeAcc_ += delta;
            if (animTimeAcc_ > duration)
                animTimeAcc_ = fmod(animTimeAcc_, duration);
        }
        // Evaluate animation at current time
        animController_.update(delta);
        const auto& boneMats = animController_.getBoneMatrices();

        // Get bind-pose positions from FBXLoader for each bone
        const auto& bones = loader_.GetBones();
        std::vector<glm::vec3> bindPositions(boneMats.size());

        for (size_t i = 0; i < boneMats.size(); ++i) {
            // Compute inverse of bindPoseInverse to retrieve original bind-pose transform
            glm::mat4 bindGlobal = glm::inverse(bones[i].bindPoseInverse);
            bindPositions[i] = glm::vec3(bindGlobal[3]); // translation from bind-pose matrix
        }

        // Build dual-quaternions for skinning
        std::vector<DualQuaternion> dqs;
        dqs.reserve(boneMats.size());
        for (const auto& M : boneMats) {
            dqs.push_back(makeDualQuat(M));
        }

        // Clear
        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use skin shader
        shader_.UseShaderProg();

        // Bind diffuse
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTex_);
        glUniform1i(glGetUniformLocation(shader_.GetProgID(), "uDiffuse"), 0);

        // Camera uniforms
        glm::mat4 view = camera_.getViewMatrix();
        glUniformMatrix4fv(
            glGetUniformLocation(shader_.GetProgID(), "uView"),
            1, GL_FALSE,
            glm::value_ptr(view)
        );
        glUniformMatrix4fv(
            glGetUniformLocation(shader_.GetProgID(), "uProj"),
            1, GL_FALSE,
            glm::value_ptr(proj_)
        );

        // Skinning mode
        glUniform1i(glGetUniformLocation(shader_.GetProgID(), "SkinningMode"), 2);

        // Upload Bind-pose
        for (GLsizei i = 0; i < (GLsizei)bindPositions.size(); ++i) {
            char name[64];
            std::snprintf(name, 64, "skeleton.bone[%d].pos", i);
            GLint loc = glGetUniformLocation(shader_.GetProgID(), name);
            glUniform3fv(loc, 1, glm::value_ptr(bindPositions[i]));
        }

        // Upload skeleton data
        mesh_.uploadSkeletonUniforms(shader_.GetProgID(), boneMats, dqs);

        // Draw
        mesh_.draw();

        // Swap
        window_.swapBuffers();
    }
}

// Static callbacks:
void Render::mouseButtonCallback(GLFWwindow* w, int b, int a, int m) {
    auto* r = static_cast<Render*>(glfwGetWindowUserPointer(w));
    double x, y;
    glfwGetCursorPos(w, &x, &y);
    r->camera_.processMouseButton(b, a, x, y);
}

void Render::cursorPosCallback(GLFWwindow* w, double x, double y) {
    auto* r = static_cast<Render*>(glfwGetWindowUserPointer(w));
    r->camera_.processCursorPos(x, y);
}

void Render::scrollCallback(GLFWwindow* w, double xoff, double yoff) {
    auto* r = static_cast<Render*>(glfwGetWindowUserPointer(w));
    r->camera_.processScroll(yoff);
}

void Render::keyCallback(GLFWwindow* w, int key, int sc, int action, int mods) {
    auto* r = static_cast<Render*>(glfwGetWindowUserPointer(w));
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        r->animController_.togglePlayback();
        std::cout << (r->animController_.isPlaying() ? "Playing\n" : "Paused\n");
    }
    if (key == GLFW_KEY_R && action == GLFW_PRESS && !r->animController_.isPlaying()) {
        r->animController_.evaluateAt(0.0);
        r->animController_.update(0.0);
        std::cout << "Set to Rest Pose" << std::endl;
    }
}
