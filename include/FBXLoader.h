#pragma once

#ifndef _FBX_LOADER_H
#define _FBX_LOADER_H

#include <fbxsdk.h>
#include <vector>
#include <glm/glm.hpp>

void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene);
void DestroySdkObjects(FbxManager* pManager, bool pExitStatus);

bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename);

void  ProcessMesh(FbxNode* node, FbxMesh*& mesh);
void GetMeshVertices(FbxMesh* mesh, std::vector<glm::vec3>& outVertices);
void GetMeshFaces(FbxMesh* mesh, std::vector<unsigned int>& outFaces);

void ProcessSkeleton(FbxNode* node, unsigned int& boneCount, int depth = 0);
void GetBoneData(FbxMesh* mesh, std::vector<std::vector<unsigned int>>& outBoneIndices, std::vector<std::vector<float>>& outBoneWeights);

void GetMeshNormals(FbxMesh* mesh, std::vector<glm::vec3>& outNormals);

glm::mat4 toGlm(const FbxAMatrix& m);

#endif // #ifndef _FBX_LOADER_H
