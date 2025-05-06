#pragma once
#include <string>
#include <vector>
#include <fbxsdk.h>
#include <glm/glm.hpp>

#ifdef IOS_REF
    #undef  IOS_REF
    #define IOS_REF (*(pManager->GetIOSettings()))
#endif

class FBXLoader {
public:
    struct FBXMeshData {
        std::vector<glm::vec3> vertices;
        std::vector<unsigned int> faces;
        std::vector<std::vector<unsigned int>> boneIndices;
        std::vector<std::vector<float>>     boneWeights;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> uvs;
    };

    struct FBXSkeleton {
        unsigned int numberOfBones = 0;
        double startTime = 0.0;
        double endTime = 0.0;
        std::vector<glm::mat4> bindPoseInverse;
        std::vector<class FbxNode*> boneNodes;
    };

    struct BoneInfluence {
        int boneIndex;
        float weight;
    };

    FBXLoader();
    ~FBXLoader();

    // Load the scene, triangulate, extract mesh & skeleton.
    bool LoadScene(const char* pFilename);

    const FBXMeshData& GetMeshData() const { return mesh_; }
    const FBXSkeleton& GetSkeletonData() const { return skeleton_; }
    const std::vector<FbxNode*>& GetSkeletonNodes() const { return skeleton_.boneNodes; }
    const std::vector<glm::mat4>& GetBindPoseInverse()   const { return skeleton_.bindPoseInverse; }
    FbxScene* GetScene() const { return pScene; }
    FbxNode* GetMeshNode() const { return meshNode_; }
    FbxPose* GetBindPose() const { return bindPose; }

    const std::vector<glm::mat4>& getBoneGlobalTransforms() const { return boneGlobals_; }
    FbxNode* getSkeletonNode(int i) { return skeleton_.boneNodes[i]; }

    static glm::mat4 fbxToGlm(const FbxAMatrix& m);

private:
    void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene);
    void DestroySdkObjects(FbxManager* pManager, bool pExitStatus);
    void triangulateScene();
    void extractMeshData(FbxNode* node);
    void extractSkeletonData(FbxNode* node);
    void getBoneData(FbxMesh* mesh);
    void computeNormals(FbxMesh* mesh);
    void computeUVs(FbxMesh* mesh);
    void getBindPose();

    // FBX SDK objects
    class FbxManager* pManager = nullptr;
    class FbxScene* pScene = nullptr;
    bool pExitStatus;

    FBXMeshData  mesh_;
    FbxNode* meshNode_ = nullptr;
    FBXSkeleton  skeleton_;
    FbxPose* bindPose = nullptr;
    std::vector<glm::mat4> boneGlobals_;
};
