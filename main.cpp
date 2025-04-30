#include <windows.h>
#include <string>
#include <limits.h>
#include <iostream>
#include <cstdlib>      // for std::getenv
#include <vector>
#include <thread>
#include <chrono>

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
#ifdef COR_ENABLE_PROFILING
            std::cout << "Number of Vertices: " << vertices.size() << std::endl;
            std::cout << "Number of Faces: " << faces.size() / 3 << std::endl;
#endif
        }
    }

    // Done reading from the .fbx file. Destroy it.
    DestroySdkObjects(lSdkManager, true);

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
     
    // Create Mesh of CoRCalculation API with conversion methods
    std::vector<CoR::WeightsPerBone> weightsPerBone = c.convertWeights(numberOfBones, boneIndices, boneWeights);
    CoR::CoRMesh corMesh = c.createCoRMesh(vertices, faces, weightsPerBone, subdivEpsilon);

    // Start asynchronously computation. In the callback we just write the cors to a file.
    c.calculateCoRsAsync(corMesh, [&c](std::vector<glm::vec3>& cors) {
        c.saveCoRsToBinaryFile("../cor_output/bfs_cs.cors", cors);
        c.saveCoRsToTextFile("../cor_output/bfs_cs.txt", cors);
        });

    // Loading can be performed from binary files only so far.
    std::vector<glm::vec3> cors = c.loadCoRsFromBinaryFile("../cor_output/output_file.cors");

    /* ======================================================================================================= */

    /* ============================================== Rendering ============================================== */
    // Initialize GLFW 
    if (!glfwInit()) { std::cerr << "GLFW init failed."; exit(1); }
    GLFWwindow* window = glfwCreateWindow(800, 600, "CoR Skinning", nullptr, nullptr);
    if (!window) { glfwTerminate(); exit(1); }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { std::cerr << "GLEW init failed\n"; exit(1); }
    glViewport(0, 0, 800, 600);
    glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    glm::vec3 camTarget(0.0f), camUp(0, 1, 0);
    float radius = 5.0f, angle = 0.0f;

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glfwDestroyWindow(window);
    glfwTerminate();
    /* ======================================================================================================= */


    return 0;
}