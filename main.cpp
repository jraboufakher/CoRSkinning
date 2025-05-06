#include <windows.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>

#include <opencv2/opencv.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "FBXLoader.h"
#include "CoRProcessor.h"
#include "render/AnimController.h"
#include "render/Window.h"
#include "render/Camera.h"
#include "render/Shader.h"
#include "render/Mesh.h"
#include "render/Render.h"

//#define CALCULATE_COR

static std::string getProjDir() {
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
    else { std::cout << "Home directory not found.\n"; }
    return path;
}

int main(int argc, char** argv) {
    // 1) Load FBX
    std::string projDir = getProjDir();
    std::string fbxFile = (argc > 1) ? argv[1]
        : projDir + R"(\input\ISO200_0003_Rigged.fbx)";

    FBXLoader loader;
    if (!loader.LoadScene(fbxFile.c_str())) {
        std::cerr << "Failed to load FBX: " << fbxFile << "\n";
        return -1;
    }
    std::cout << "Scene loaded successfully\n";

    const auto& meshData = loader.GetMeshData();
    const auto& skelData = loader.GetSkeletonData();

#ifdef DEBUG
    std::cout << "Verts: " << meshData.vertices.size()
        << ", Tris: " << meshData.faces.size() / 3
        << ", Bones: " << skelData.numberOfBones
        << " (" << skelData.startTime << "–" << skelData.endTime << "s)\n";
#endif

    CoRProcessor corProc(
        /*sigma*/0.1f,
        /*omega*/0.1f,
        /*performSubdivision*/false,
        /*numThreads*/8
    );

#ifdef CALCULATE_COR
    // Async compute and write out
    corProc.computeCoRsAsync(
        meshData,
        skelData.numberOfBones,
        nullptr  // no further callback
    );
    // give it a moment (in your real app you’d wait on an event)
    std::this_thread::sleep_for(std::chrono::seconds(1));
#endif

    // Load the precomputed centers
    std::string corsFile = projDir + R"(\cor_output\output_file.cors)";
    auto cors = corProc.LoadCoRsFromBinaryFile(corsFile);

#ifdef DEBUG
    if (cors.size() != meshData.vertices.size()) {
        std::cerr << "ERROR: mismatched CoR count\n";
        return -1;
    }
#endif

    // Pack into Mesh
    Mesh mesh;
    mesh.positions = meshData.vertices;
    mesh.normals = meshData.normals;
    mesh.uvs = meshData.uvs;
    mesh.indices = meshData.faces;
    mesh.centersOfRotation = cors;

    mesh.skinInfo.resize(mesh.positions.size());
    for (size_t i = 0; i < mesh.positions.size(); ++i) {
        auto& dst = mesh.skinInfo[i];
        const auto& bi = meshData.boneIndices[i];
        const auto& bw = meshData.boneWeights[i];
        float sum = 0.f;
        for (int j = 0; j < MAX_INFLUENCES; ++j) {
            if (j < (int)bi.size()) {
                dst.boneIDs[j] = bi[j];
                dst.weights[j] = bw[j];
            }
            else {
                dst.boneIDs[j] = 0;
                dst.weights[j] = 0.f;
            }
            sum += dst.weights[j];
        }
        if (sum > 0.f) {
            for (int j = 0; j < MAX_INFLUENCES; ++j)
                dst.weights[j] /= sum;
        }
        else {
            dst.boneIDs[0] = 0;
            dst.weights[0] = 1.f;
        }
    }

    // Renderer
    Window  window(800, 600, "CoR Skinning");
    Camera  camera;
    Shader  shader;
    AnimController animController;

    // diffuse map
    std::string texPath = projDir + R"(\input\ISO200_0003_Model_11_u1_v1_diffuse.png)";

    Render renderer(window, camera, shader, mesh, loader, animController, texPath);
    if (!renderer.InitializeRender()) {
        std::cerr << "Failed to initialize renderer\n";
        return -1;
    }

    renderer.run();
    return 0;
}
