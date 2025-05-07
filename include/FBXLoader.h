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

    struct Bone {
        FbxNode* node;
        int parentIndex;
        glm::mat4 bindPoseInverse;
    };

    struct FBXSkeleton {
        unsigned int numberOfBones = 0;
        std::vector<Bone> bones;
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
    const std::vector<Bone>& GetBones() const { return skeleton_.bones; }

    const std::vector<FbxNode*> GetSkeletonNodes() const {
        std::vector<FbxNode*> nodes;
        nodes.reserve(skeleton_.bones.size());
        for (const auto& bone : skeleton_.bones) {
            nodes.push_back(bone.node);
        }
        return nodes;
    }

    const std::vector<glm::mat4> GetBindPoseInverse() const {
        std::vector<glm::mat4> inverses;
        inverses.reserve(skeleton_.bones.size());
        for (const auto& bone : skeleton_.bones) {
            inverses.push_back(bone.bindPoseInverse);
        }
        return inverses;
    }

    FbxScene* GetScene() const { return pScene; }
    FbxNode* GetMeshNode() const { return meshNode_; }

    const std::vector<glm::mat4>& getBoneGlobalTransforms() const { return boneGlobals_; }

    static glm::mat4 fbxToGlm(const FbxAMatrix& m);

private:
    void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene);
    void DestroySdkObjects(FbxManager* pManager, bool pExitStatus);
    void triangulateScene();
    void extractMeshData(FbxNode* node);
    void extractSkeletonData();
    void extractSkeletonRecursive(FbxNode* node, int parentIndex);
    void getBoneData(FbxMesh* mesh);
    void computeNormals(FbxMesh* mesh);
    void computeUVs(FbxMesh* mesh);

    // FBX SDK objects
    class FbxManager* pManager = nullptr;
    class FbxScene* pScene = nullptr;
    bool pExitStatus;

    FBXMeshData  mesh_;
    FbxNode* meshNode_ = nullptr;
    FBXSkeleton  skeleton_;
    std::vector<glm::mat4> boneGlobals_;
};
