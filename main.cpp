#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>

#include "FBXLoader.h"
#include "cor/CoRCalculator.h"
#include "cor/CoRMesh.h"
#include "cor/ShaderUtils.h"
#include "cor/Mesh.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

// For Rendering
#include <GL/glew.h>
#include <GLFW/glfw3.h>

static bool gVerbose = true;

std::string getProjDir() {
    char buff[MAX_PATH];
    GetModuleFileName(NULL, buff, MAX_PATH);
    std::string::size_type position = std::string(buff).find_last_of("\\/");
    std::string path = std::string(buff).substr(0, position);

    if (path == "") {
        std::cerr << "Current directory not found." << std::endl;
        exit(1);
    }

    size_t pos = path.find(R"(\build)");
    if (pos != std::string::npos) {
        path = path.substr(0, pos);
    }
    else {
        std::cout << "Home directory not found.\n";
    }
         
    return path;
}

// --- camera state ---
float camYaw = 0.0f;           // left/right angle in radians
float camPitch = 0.0f;           // up/down angle in radians
float camRadius = 3.0f;           // distance from target
glm::vec3 camTarget = glm::vec3(0, 1, 0); // orbit origin

bool  rotating = false;
bool dragging = false;
double lastX, lastY;                // last mouse positions

// sensitivity
const float yawSpeed = 0.005f;
const float pitchSpeed = 0.005f;
const float zoomSpeed = 1.0f;
const float dragSpeed = 0.005f;


// called when a mouse button is pressed/released
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            rotating = true;
            glfwGetCursorPos(window, &lastX, &lastY);
        }
        else if (action == GLFW_RELEASE) {
            rotating = false;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            dragging = true;
            glfwGetCursorPos(window, &lastX, &lastY);
        }
        else if (action == GLFW_RELEASE) {
            dragging = false;
        }
    }
    
}

// called whenever the cursor moves
void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    float dx = float(xpos - lastX);
    float dy = float(ypos - lastY);

    // rotating
    if (rotating) {
        camYaw += dx * yawSpeed;
        camPitch += dy * pitchSpeed;
        camPitch = glm::clamp(camPitch, -glm::radians(89.0f), glm::radians(89.0f));
    }
    // dragging
    else if (dragging) {
        float x = camRadius * cos(camPitch) * sin(camYaw);
        float y = camRadius * sin(camPitch);
        float z = camRadius * cos(camPitch) * cos(camYaw);

        glm::vec3 camPos = glm::vec3(x, y, z) + camTarget;
        glm::vec3 forward = glm::normalize(camTarget - camPos);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
        glm::vec3 up = glm::normalize(glm::cross(right, forward));

        camTarget -= right * dx * dragSpeed;
        camTarget += up * dy * dragSpeed;
    }
    lastX = xpos; lastY = ypos;
}

// called on scroll-wheel
void scroll_callback(GLFWwindow* window, double /*xoffset*/, double yoffset) {
    camRadius -= float(yoffset) * zoomSpeed;
    // prevent going through the target:
    camRadius = glm::clamp(camRadius, 1.0f, 100.0f);
}

int main(int argc, char** argv) {
    // FBX 
    std::string filePath = getProjDir() + "\\input\\ISO200_0003_Rigged.fbx";

    FbxManager* lSdkManager = NULL;
    FbxScene* lScene = NULL;
    FbxMesh* rootMesh = NULL;
    bool lResult;

    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> faces;
    std::vector<std::vector<unsigned int>>boneIndices;
    std::vector<std::vector<float>> boneWeights;
    unsigned int numberOfBones = 0;

    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;

    // Prepare the FBX SDK.
    InitializeSdkObjects(lSdkManager, lScene);
    // Load the scene.

    // Read in FBX file and check that it was successfully read in.
    FbxString lFilePath(filePath.c_str());
    for (int i = 1, c = argc; i < c; ++i)
    {
        if (FbxString(argv[i]) == "-test")
            gVerbose = false;
        else if (lFilePath.IsEmpty())
            lFilePath = argv[i];
    }
    if (lFilePath.IsEmpty())
    {
        lResult = false;
        FBXSDK_printf("\n\nUsage: ImportScene <FBX file name>\n\n");
    }
    else
    {
        FBXSDK_printf("\n\nFile: %s\n\n", lFilePath.Buffer());
        lResult = LoadScene(lSdkManager, lScene, lFilePath.Buffer());
    }

    if (lResult == false)
    {
        FBXSDK_printf("\n\nAn error occurred while loading the scene...");
    }
    else
    {
        // FBX read in successfully, continue.
        printf("Scene loaded successfully\n");

        FbxNode* rootNode = lScene->GetRootNode();
        if (rootNode)
        {
            // Extract Mesh
            ProcessMesh(rootNode, rootMesh);

            // Vertices
            GetMeshVertices(rootMesh, vertices);

            // Faces
            GetMeshFaces(rootMesh, faces);

            // Number of Bones
            ProcessSkeleton(rootNode, numberOfBones);

            // Bone Weights and Indices
            GetBoneData(rootMesh, boneIndices, boneWeights);

            // Normals
            GetMeshNormals(rootMesh, normals);
            if (normals.size() != vertices.size()) {
                std::cerr << "Normal count mismatch: " << normals.size()
                    << " vs " << vertices.size() << "\n";
            }

            // UVs
            GetMeshUVs(rootMesh, uvs);
            if (uvs.size() != vertices.size()) {
                std::cerr << "UV count mismatch: " << uvs.size()
                    << " vs " << vertices.size() << "\n";
            }

#ifdef COR_ENABLE_PROFILING
            std::cout << "Number of Vertices: " << vertices.size() << std::endl;
            std::cout << "Number of Faces: " << faces.size() / 3 << std::endl;
            std::cout << "Number of Normals: " << normals.size() << std::endl;
            std::cout << "Number of UVs: " << uvs.size() << std::endl;
#endif
        }
    }

    // SKELETON STATE
    int numBones = static_cast<int>(numberOfBones);

    // FbxNode* for each skeleton bone:
    std::vector<FbxNode*> boneNodes;
    std::function<void(FbxNode*)> collectBones = [&](FbxNode* node) {
        if (node->GetNodeAttribute() &&
            node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
            boneNodes.push_back(node);
        for (int i = 0; i < node->GetChildCount(); ++i)
            collectBones(node->GetChild(i));
        };
    collectBones(lScene->GetRootNode());   // boneNodes.size() == numBones

    // Precompute each bone’s inverse bind‐pose:
    std::vector<glm::mat4> bindPoseInverse(numBones);
    for (int i = 0; i < numBones; ++i) {
        // evaluate the bone’s global transform at time = 0
        FbxTime t0; t0.SetSecondDouble(0.0);
        FbxAMatrix bindGlobal = boneNodes[i]->EvaluateGlobalTransform(t0);
        bindPoseInverse[i] = glm::inverse(fbxToGlm(bindGlobal));
    }

    // Per‐frame arrays:
    std::vector<glm::mat4> boneMatrices(numBones, glm::mat4(1.0f));
    std::vector<glm::quat> boneQuaternions(numBones, glm::quat(1, 0, 0, 0));

    /* ================================================= CoR ================================================= */
    // Choose computation values
    float subdivEpsilon = 0.5f;
    float sigma = 0.1f;
    float omega = 0.1f;
    float bfsEpsilon = 0.000001f;
    unsigned int numberOfThreadsToUse = 8;
    bool performSubdivision = false;

    // Choose one
    CoR::CoRCalculator c(sigma, omega, performSubdivision, numberOfThreadsToUse);
    //CoR::BFSCoRCalculator c(sigma, omega, performSubdivision, numberOfThreadsToUse, bfsEpsilon);
     
#ifdef CALCULATE_COR
    // Create Mesh of CoRCalculation API with conversion methods
    std::vector<CoR::WeightsPerBone> weightsPerBone = c.convertWeights(numberOfBones, boneIndices, boneWeights);
    CoR::CoRMesh corMesh = c.createCoRMesh(vertices, faces, weightsPerBone, subdivEpsilon);

    // Start asynchronously computation. In the callback we just write the cors to a file.
    c.calculateCoRsAsync(corMesh, [&c](std::vector<glm::vec3>& cors) {
        c.saveCoRsToBinaryFile("../cor_output/bfs_cs.cors", cors);
        c.saveCoRsToTextFile("../cor_output/bfs_cs.txt", cors);
        });
#endif // CALCULATE_COR

    // Loading can be performed from binary files only so far.
    std::vector<glm::vec3> cors = c.loadCoRsFromBinaryFile("../cor_output/output_file.cors");

#ifdef COR_ENABLE_PROFILING
    for (int i = 0; i < std::min(5, int(vertices.size())); ++i) {
        std::cout << "CoR[" << i << "] = "
            << cors[i].x << ", "
            << cors[i].y << ", "
            << cors[i].z << "\n";
    }
    // Check that cors has the same number of entries as vertices.
    if (cors.size() != vertices.size()) {
        std::cerr << "ERROR: cors has " << cors.size()
            << " entries, but mesh has "
            << vertices.size() << " vertices!\n";
        return -1;
    }
#endif
    /* ======================================================================================================= */

    /* ============================================== Rendering ============================================== */
    // Initialize GLFW 
    if (!glfwInit()) { std::cerr << "GLFW init failed."; exit(1); }
    GLFWwindow* window = glfwCreateWindow(800, 600, "CoR Skinning", nullptr, nullptr);
    if (!window) { glfwTerminate(); exit(1); }
    glfwMakeContextCurrent(window);
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { std::cerr << "GLEW init failed\n"; exit(1); }

    glViewport(0, 0, 800, 600);
    glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);

    Mesh mesh;
    mesh.positions = vertices;
    mesh.normals = normals;
    mesh.indices = faces;
    mesh.uvs = uvs;

    size_t numberOfVertices = vertices.size();

    mesh.skinInfo.resize(numberOfVertices);
    mesh.centersOfRotation = cors;
    for (size_t i = 0; i < numberOfVertices; ++i) {
        auto& info = mesh.skinInfo[i];

        // copy up to 4, pad the rest with zeros
        const auto& bi = boneIndices[i];
        const auto& bw = boneWeights[i];
        for (int j = 0; j < 4; ++j) {
            if (j < (int)bi.size()) {
                info.boneIDs[j] = bi[j];
                info.weights[j] = bw[j];
            }
            else {
                info.boneIDs[j] = 0;
                info.weights[j] = 0.f;
            }
        }
        // now normalize so weights sum to 1.0
        float sum = info.weights[0]
            + info.weights[1]
            + info.weights[2]
            + info.weights[3];
        if (sum > 0.f) {
            for (int j = 0; j < 4; ++j)
                info.weights[j] /= sum;
        }
        else {
            // fallback – bind to bone 0
            info.boneIDs[0] = 0;
            info.weights[0] = 1.f;
        }
    }

    // Texture
    GLuint diffuseTex;
    glGenTextures(1, &diffuseTex);
    glBindTexture(GL_TEXTURE_2D, diffuseTex);

    // load the texture map
    int w, h, channels;
    cv::Mat textureData_bgr = cv::imread(getProjDir() + "\\input\\ISO200_0003_Rigged.fbm\\ISO200_0003_Model_11_u1_v1_diffuse.jpg", cv::IMREAD_COLOR);
    if (textureData_bgr.empty()) {
        std::cerr << "Could not open or find the texture map." << std::endl;
        return -1;
    }
    cv::Mat textureData;
    cv::cvtColor(textureData_bgr, textureData, cv::COLOR_BGR2RGBA);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);         // safe row alignment

    // allocate + upload
    glTexImage2D(
        GL_TEXTURE_2D,
        0,                   // base mip level
        GL_RGBA8,            // internal format
        textureData.cols,    // width
        textureData.rows,    // height
        0,                   // border
        GL_RGBA,             // source format
        GL_UNSIGNED_BYTE,    // source type
        textureData.data     // pointer to pixel data
    );

    // mipmaps + filtering
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    mesh.initBuffers();

#ifdef COR_ENABLE_PROFILING
    std::cout << "good so far :: after Mesh" << std::endl;
#endif
    // Shaders
    GLuint skinProg = LoadShaders(
        "../shaders/skin.vert",
        "../shaders/skin.frag"
    );
    if (!skinProg) {
        std::cerr << "Failed to load skin shaders\n";
        return -1;
    }

#ifdef COR_ENABLE_PROFILING
    std::cout << "Good after Shaders." << std::endl;
#endif

    glm::mat4 proj = glm::perspective(
        glm::radians(45.0f), // fov
        800.0f / 600.0f,       // aspect
        0.1f, 100.0f         // near, far
    );

    // the render loop
    while (!glfwWindowShouldClose(window)) {
        // Handle OS events (keyboard, mouse, window resize…)
        glfwPollEvents();

        // Compute camera position from spherical coords
        float x = camRadius * cos(camPitch) * sin(camYaw);
        float y = camRadius * sin(camPitch);
        float z = camRadius * cos(camPitch) * cos(camYaw);
        glm::vec3 camPos = glm::vec3(x, y, z) + camTarget;

        // View matrix
        glm::mat4 view = glm::lookAt(
            camPos,          // camera in world
            camTarget,       // looking at origin (or any point)
            glm::vec3(0, 1, 0) // world’s up-vector
        );

        // Rotate the view so the model is upright
        view = view * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        glUniformMatrix4fv(
            glGetUniformLocation(skinProg, "uView"),
            1, GL_FALSE,
            glm::value_ptr(view)
        );

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Compute your current animation pose (update boneMatrices & boneQuaternions each frame)
        FbxTime fbxTime;
        fbxTime.SetSecondDouble(glfwGetTime());  // current animation time

        for (int i = 0; i < numBones; ++i) {
            // get the bone’s global XF at this time
            FbxAMatrix globalFbx = boneNodes[i]->EvaluateGlobalTransform(fbxTime);
            glm::mat4 globalMat = fbxToGlm(globalFbx);

            // skinning matrix = global * inverse_bind
            boneMatrices[i] = globalMat * bindPoseInverse[i];

            // extract the rotation
            boneQuaternions[i] = glm::quat_cast(glm::mat3(boneMatrices[i]));
        }

        // Start skinning shader
        glUseProgram(skinProg);

        // Bind textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTex);

        GLint locDiffuse = glGetUniformLocation(skinProg, "uDiffuse");
        glUniform1i(locDiffuse, 0);

        // Upload camera uniforms
        GLint locView = glGetUniformLocation(skinProg, "uView");
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));

        GLint locProj = glGetUniformLocation(skinProg, "uProj");
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(proj));

        // Set skinning mode (CoR is mode 2)
        GLint locMode = glGetUniformLocation(skinProg, "SkinningMode");
        glUniform1i(locMode, 2);

        // Upload each bone’s transform and quaternion
        for (int i = 0; i < numBones; ++i) {
            // bone matrix
            char nameMat[32];
            sprintf(nameMat, "uBoneMatrices[%d]", i);
            GLint locMat = glGetUniformLocation(skinProg, nameMat);
            glUniformMatrix4fv(
                locMat, 1, GL_FALSE,
                glm::value_ptr(boneMatrices[i])
            );

            // bone quaternion
            char nameQuat[32];
            sprintf(nameQuat, "uBoneQuaternions[%d]", i);
            GLint locQuat = glGetUniformLocation(skinProg, nameQuat);
            glUniform4fv(
                locQuat, 1,
                glm::value_ptr(boneQuaternions[i])
            );
        }

        // Draw mesh (binds VAO + draw call)
        mesh.draw();

        glfwSwapBuffers(window);
    }

    // after the loop: cleanup
    glDeleteProgram(skinProg);
    glfwDestroyWindow(window);
    glfwTerminate();

    glfwDestroyWindow(window);
    glfwTerminate();
    /* ======================================================================================================= */

    DestroySdkObjects(lSdkManager, true);

    return 0;
}