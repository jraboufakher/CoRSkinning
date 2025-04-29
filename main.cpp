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

    GLuint skinProg = LoadShaders("C:/Users/Jana/Desktop/CoRSkinning/shaders/skin.vert", "C:/Users/Jana/Desktop/CoRSkinning/shaders/skin.frag");
    if (!skinProg) {
        std::cerr << "Failed to create skin shader program\n";
        exit(1);
    }

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
    bool performSubdivision = true;

    // Choose one
    // CoR::CoRCalculator c(sigma, omega, performSubdivision, numberOfThreadsToUse);
    CoR::BFSCoRCalculator cs(sigma, omega, performSubdivision, numberOfThreadsToUse, bfsEpsilon);
     
    // Create Mesh of CoRCalculation API with conversion methods
    std::vector<CoR::WeightsPerBone> weightsPerBone = cs.convertWeights(numberOfBones, boneIndices, boneWeights);
    CoR::CoRMesh corMesh = cs.createCoRMesh(vertices, faces, weightsPerBone, subdivEpsilon);

    std::cout << "Passed corMesh. calculateCoRsAsync next." << std::endl;
    // Start asynchronously computation. In the callback we just write the cors to a file.
    cs.calculateCoRsAsync(corMesh, [&cs](std::vector<glm::vec3>& cors) {
        cs.saveCoRsToBinaryFile("../cor_output/bfs_cs.cors", cors);
        cs.saveCoRsToTextFile("../cor_output/bfs_cs.txt", cors);
        });

    // Loading can be performed from binary files only so far.
    std::vector<glm::vec3> cors = cs.loadCoRsFromBinaryFile("../cor_output/output_file.cors");

    /* ======================================================================================================= */

    ///* ============================================== Rendering ============================================== */
    //std::vector<VertexSkinData> skinInfo(vertices.size());
    //for (size_t i = 0; i < vertices.size(); ++i) {
    //    for (int j = 0; j < MAX_INFLUENCES; ++j) {
    //        if (j < (int)boneIndices[i].size()) {
    //            skinInfo[i].boneIDs[j] = boneIndices[i][j];
    //            skinInfo[i].weights[j] = boneWeights[i][j];
    //        }
    //        else {
    //            skinInfo[i].boneIDs[j] = 0;
    //            skinInfo[i].weights[j] = 0.0f;
    //        }
    //    }
    //}

    //// Create & fill the Mesh
    //Mesh mesh;
    //mesh.positions = vertices;
    //mesh.normals = normals;
    //mesh.centersOfRotation = cors;
    //mesh.skinInfo = skinInfo;
    //mesh.indices = faces;

    //// Upload to GPU
    //mesh.initBuffers();

    //// 1) Gather bind‐pose: before you deform at all
    //std::vector<glm::mat4> invBind(numberOfBones);
    //for (int i = 0; i < numberOfBones; ++i) {
    //    FbxNode* boneNode = /* your skeleton node i */;
    //    FbxAMatrix bind = boneNode->EvaluateGlobalTransform(/* at time=0 */);
    //    glm::mat4 bindMat = toGlm(bind);                  // your utility
    //    invBind[i] = glm::inverse(bindMat);
    //}

    //// 2) Each frame, get current pose
    //std::vector<glm::mat4> boneMatrices(numberOfBones);
    //for (int i = 0; i < numberOfBones; ++i) {
    //    FbxNode* boneNode = /* your skeleton node i */;
    //    FbxAMatrix curr = boneNode->EvaluateGlobalTransform(currentTime);
    //    glm::mat4 currMat = toGlm(curr);
    //    boneMatrices[i] = currMat * invBind[i];
    //}

    //// 3) Then build dual‐quats and upload
    //boneDualQuats.clear();
    //for (auto& M : boneMatrices)
    //    boneDualQuats.push_back(makeDualQuat(M));


    //while (!glfwWindowShouldClose(window)) {
    //    angle += 0.5f;                              // advance your orbit
    //    float rad = glm::radians(angle);
    //    glm::vec3 camPos = {
    //        radius * std::sin(rad),
    //        2.0f,
    //        radius * std::cos(rad)
    //    };
    //    glm::mat4 view = glm::lookAt(camPos, camTarget, camUp);
    //    glm::mat4 proj = glm::perspective(
    //        glm::radians(45.0f),
    //        800.0f / 600.0f,
    //        0.1f,
    //        100.0f
    //    );
    //    glm::mat4 projView = proj * view;

    //    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //    // 5.1) use your skinning shader
    //    glUseProgram(skinProg);

    //    // 5.2) tell shader to use CoR‐skinning
    //    glUniform1i(glGetUniformLocation(skinProg, "SkinningMode"), 2);

    //    // 5.3) set uProjView
    //    glUniformMatrix4fv(
    //        glGetUniformLocation(skinProg, "uProjView"),
    //        1, GL_FALSE,
    //        glm::value_ptr(projView)
    //    );

    //    // 5.4) upload your skeleton uniforms:
    //    //      skeleton.numBones, skeleton.bone[i].transform
    //    //      and skeleton.bone[i].dqTransform.real/dual
    //    mesh.uploadSkeletonUniforms(skinProg, boneMatrices, boneDualQuats);

    //    // 5.5) draw your mesh
    //    mesh.draw();

    //    // 5.6) swap & poll
    //    glfwSwapBuffers(window);
    //    glfwPollEvents();
    //}

    ///* ======================================================================================================= */

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}