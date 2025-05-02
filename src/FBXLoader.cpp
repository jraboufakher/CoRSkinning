#include <iostream>
#include <vector>
#include <map>
#include <cstdint>
#include <algorithm>
#include <fstream>

#include "FBXLoader.h"
#include "DisplayCommon.h"

struct BoneInfluence {
    int boneIndex;
    float weight;
};

using VertexSkinMap = std::map<int, std::vector<BoneInfluence>>; // maps vertex index -> list of (bone index, weight)

#ifdef IOS_REF
    #undef  IOS_REF
    #define IOS_REF (*(pManager->GetIOSettings()))
#endif

void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene)
{
    //The first thing to do is to create the FBX Manager which is the object allocator for almost all the classes in the SDK
    pManager = FbxManager::Create();
    if (!pManager)
    {
        FBXSDK_printf("Error: Unable to create FBX Manager!\n");
        exit(1);
    }
    else FBXSDK_printf("Autodesk FBX SDK version %s\n", pManager->GetVersion());

    //Create an IOSettings object. This object holds all import/export settings.
    FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
    pManager->SetIOSettings(ios);

    //Load plugins from the executable directory (optional)
    FbxString lPath = FbxGetApplicationDirectory();
    pManager->LoadPluginsDirectory(lPath.Buffer());

    //Create an FBX scene. This object holds most objects imported/exported from/to files.
    pScene = FbxScene::Create(pManager, "My Scene");
    if (!pScene)
    {
        FBXSDK_printf("Error: Unable to create FBX scene!\n");
        exit(1);
    }
}

void DestroySdkObjects(FbxManager* pManager, bool pExitStatus)
{
    //Delete the FBX Manager. All the objects that have been allocated using the FBX Manager and that haven't been explicitly destroyed are also automatically destroyed.
    if (pManager) pManager->Destroy();
    if (pExitStatus) FBXSDK_printf("Program Success!\n");
}

bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename)
{
    int lFileMajor, lFileMinor, lFileRevision;
    int lSDKMajor, lSDKMinor, lSDKRevision;
    //int lFileFormat = -1;
    int lAnimStackCount;
    bool lStatus;

    // Get the file version number generate by the FBX SDK.
    FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

    // Create an importer.
    FbxImporter* lImporter = FbxImporter::Create(pManager, "");

    // Initialize the importer by providing a filename.
    const bool lImportStatus = lImporter->Initialize(pFilename, -1, pManager->GetIOSettings());
    lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

    if (!lImportStatus)
    {
        FbxString error = lImporter->GetStatus().GetErrorString();
        FBXSDK_printf("Call to FbxImporter::Initialize() failed.\n");
        FBXSDK_printf("Error returned: %s\n\n", error.Buffer());

        if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
        {
            FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);
            FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);
        }

        return false;
    }

    FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);

    if (lImporter->IsFBX())
    {
        FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);

        // From this point, it is possible to access animation stack information without
        // the expense of loading the entire file.

        FBXSDK_printf("Animation Stack Information\n");

        lAnimStackCount = lImporter->GetAnimStackCount();

        FBXSDK_printf("    Number of Animation Stacks: %d\n", lAnimStackCount);
        FBXSDK_printf("    Current Animation Stack: \"%s\"\n", lImporter->GetActiveAnimStackName().Buffer());
        FBXSDK_printf("\n");

        for (int i = 0; i < lAnimStackCount; i++)
        {
            FbxTakeInfo* lTakeInfo = lImporter->GetTakeInfo(i);

            FBXSDK_printf("    Animation Stack %d\n", i);
            FBXSDK_printf("         Name: \"%s\"\n", lTakeInfo->mName.Buffer());
            FBXSDK_printf("         Description: \"%s\"\n", lTakeInfo->mDescription.Buffer());

            // Change the value of the import name if the animation stack should be imported 
            // under a different name.
            FBXSDK_printf("         Import Name: \"%s\"\n", lTakeInfo->mImportName.Buffer());

            // Set the value of the import state to false if the animation stack should be not
            // be imported. 
            FBXSDK_printf("         Import State: %s\n", lTakeInfo->mSelect ? "true" : "false");
            FBXSDK_printf("\n");
        }

        // Set the import states. By default, the import states are always set to 
        // true. The code below shows how to change these states.
        IOS_REF.SetBoolProp(IMP_FBX_MATERIAL, true);
        IOS_REF.SetBoolProp(IMP_FBX_TEXTURE, true);
        IOS_REF.SetBoolProp(IMP_FBX_LINK, true);
        IOS_REF.SetBoolProp(IMP_FBX_SHAPE, true);
        IOS_REF.SetBoolProp(IMP_FBX_GOBO, true);
        IOS_REF.SetBoolProp(IMP_FBX_ANIMATION, true);
        IOS_REF.SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
    }

    // Import the scene.
    lStatus = lImporter->Import(pScene);
    if (lStatus == true)
    {
        // Check the scene integrity!
        FbxStatus status;
        FbxArray< FbxString*> details;
        FbxSceneCheckUtility sceneCheck(FbxCast<FbxScene>(pScene), &status, &details);
        lStatus = sceneCheck.Validate(FbxSceneCheckUtility::eCkeckData);
        if (lStatus == false)
        {
            if (details.GetCount())
            {
                FBXSDK_printf("Scene integrity verification failed with the following errors:\n");
                for (int i = 0; i < details.GetCount(); i++)
                    FBXSDK_printf("   %s\n", details[i]->Buffer());

                FbxArrayDelete<FbxString*>(details);
            }
        }
    }

    // Destroy the importer.
    lImporter->Destroy();

    return lStatus;
}

// Process the scene to look for FbxSkeleton nodes.
void ProcessSkeleton(FbxNode* node, unsigned int& boneCount, int depth)
{
    if (!node) return;

    FbxNodeAttribute* attr = node->GetNodeAttribute();
    if (attr && attr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
    {
        std::string name = node->GetName();
        if (name.size() >= 4 && name.substr(name.size() - 4) == "_end")
        {
            return; // skip "_end" bones
        }
        boneCount++;

        // for (int i = 0; i < depth; i++) std::cout << "  ";
        // std::cout << "Joint: " << name;

        auto skeleton = static_cast<FbxSkeleton*>(attr);
        /*switch (skeleton->GetSkeletonType()) {
        case FbxSkeleton::eRoot: std::cout << " (Root)"; break;
        case FbxSkeleton::eLimb: std::cout << " (Limb)"; break;
        case FbxSkeleton::eLimbNode: std::cout << " (Limb Node)"; break;
        case FbxSkeleton::eEffector: std::cout << " (Effector)"; break;
        }
        std::cout << std::endl;*/

        FbxAMatrix globalTransform = node->EvaluateGlobalTransform();
        FbxVector4 T = globalTransform.GetT();
        // std::cout << "    Global Translation: (" << T[0] << ", " << T[1] << ", " << T[2] << ")\n";
    }

    for (int i = 0; i < node->GetChildCount(); ++i)
    {
        ProcessSkeleton(node->GetChild(i), boneCount, depth + 1);
    }
}


void GetBoneData(FbxMesh* mesh, std::vector<std::vector<unsigned int>>& outBoneIndices, std::vector<std::vector<float>>& outBoneWeights)
{
    if (!mesh) return;

    int controlPointCount = mesh->GetControlPointsCount();
    std::map<int, std::vector<BoneInfluence>> skinWeights;

    int skinCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
    if (skinCount == 0)
        return;

    // Influences per control point (bone weight per vertex)
    for (int i = 0; i < skinCount; i++) {
        FbxSkin* skin = static_cast<FbxSkin*>(mesh->GetDeformer(i, FbxDeformer::eSkin));
        int clusterCount = skin->GetClusterCount();

        for (int j = 0; j < clusterCount; j++) {
            FbxCluster* cluster = skin->GetCluster(j);
            int* indices = cluster->GetControlPointIndices();
            double* weights = cluster->GetControlPointWeights();
            int count = cluster->GetControlPointIndicesCount();

            for (int k = 0; k < count; k++) {
                int cpIndex = indices[k];
                float weight = static_cast<float>(weights[k]);

                if (weight > 0.0f) {
                    skinWeights[cpIndex].push_back({ j, weight }); // j = bone index
                }
            }
        }
    }
    outBoneIndices.resize(controlPointCount);
    outBoneWeights.resize(controlPointCount);

    // Normalize and fill output
    for (int cpIndex = 0; cpIndex < controlPointCount; ++cpIndex) {
        auto& influences = skinWeights[cpIndex];

        std::sort(influences.begin(), influences.end(), [](const BoneInfluence& a, const BoneInfluence& b) {
            return a.weight > b.weight;
            });

        float total = 0.0f;
        for (const auto& inf : influences)
            total += inf.weight;

        if (total > 0.0f) {
            for (const auto& inf : influences) {
                outBoneIndices[cpIndex].push_back(static_cast<unsigned int>(inf.boneIndex));
                outBoneWeights[cpIndex].push_back(inf.weight / total);
            }
        }
    }
}

void ProcessMesh(FbxNode* node, FbxMesh* &mesh)
{
    FbxNodeAttribute* attr = node->GetNodeAttribute();
    if (attr && attr->GetAttributeType() == FbxNodeAttribute::eMesh)
    {
        mesh = node->GetMesh();
        return;    
    }
    
    for (int i = 0; i < node->GetChildCount(); ++i)
    {
        //ProcessMesh(node->GetChild(i), mesh, outBoneIndices, outBoneWeights);
        ProcessMesh(node->GetChild(i), mesh);
    }
}

void GetMeshVertices(FbxMesh* mesh, std::vector<glm::vec3>& outVertices) {
    if (!mesh) return;

    int controlPointCount = mesh->GetControlPointsCount();
    FbxVector4* controlPoints = mesh->GetControlPoints();

    outVertices.reserve(controlPointCount);

    for (int i = 0; i < controlPointCount; ++i) {
        FbxVector4& p = controlPoints[i];
        glm::vec3 vertex(static_cast<float>(p[0]),
            static_cast<float>(p[1]),
            static_cast<float>(p[2]));
        outVertices.push_back(vertex);
    }
}

void GetMeshFaces(FbxMesh* mesh, std::vector<unsigned int>& outFaces) {
    if (!mesh) return;

    int polygonCount = mesh->GetPolygonCount();
    outFaces.reserve(polygonCount * 3);

    // Loop over each polygon in the mesh.
    for (int polygonIndex = 0; polygonIndex < polygonCount; ++polygonIndex) {
        int vertexCount = mesh->GetPolygonSize(polygonIndex);

        // Skip degenerate polygons
        if (vertexCount < 3) {
            printf("Warning: Polygon %d has less than 3 vertices (%d)\n",
                polygonIndex, vertexCount);
            continue;
        }
        // If the polygon is a triangle, simply add its 3 vertices.
        else if (vertexCount == 3) {
            for (int vertexIndex = 0; vertexIndex < 3; ++vertexIndex) {
                int controlPointIndex = mesh->GetPolygonVertex(polygonIndex, vertexIndex);
                outFaces.push_back(static_cast<unsigned int>(controlPointIndex));
            }
        }
        // For polygons with more than 3 vertices, use a triangle fan to triangulate.
        else {
            int v0 = mesh->GetPolygonVertex(polygonIndex, 0);
            for (int vertexIndex = 1; vertexIndex < vertexCount - 1; ++vertexIndex) {
                int v1 = mesh->GetPolygonVertex(polygonIndex, vertexIndex);
                int v2 = mesh->GetPolygonVertex(polygonIndex, vertexIndex + 1);
                outFaces.push_back(static_cast<unsigned int>(v0));
                outFaces.push_back(static_cast<unsigned int>(v1));
                outFaces.push_back(static_cast<unsigned int>(v2));
            }
        }
    }
}

void GetMeshNormals(FbxMesh* mesh, std::vector<glm::vec3>& outNormals) {
    if (!mesh) return;
    int polyCount = mesh->GetPolygonCount();
    outNormals.resize(mesh->GetControlPointsCount(), glm::vec3(0));
    // average per‐vertex normals:
    for (int pi = 0; pi < polyCount; ++pi) {
        // fetch the three control‐point indices
        int i0 = mesh->GetPolygonVertex(pi, 0),
            i1 = mesh->GetPolygonVertex(pi, 1),
            i2 = mesh->GetPolygonVertex(pi, 2);
        auto p0 = mesh->GetControlPointAt(i0),
            p1 = mesh->GetControlPointAt(i1),
            p2 = mesh->GetControlPointAt(i2);
        glm::vec3 v0(p0[0], p0[1], p0[2]),
            v1(p1[0], p1[1], p1[2]),
            v2(p2[0], p2[1], p2[2]);
        glm::vec3 faceNorm = glm::normalize(glm::cross(v1 - v0, v2 - v0));
        outNormals[i0] += faceNorm;
        outNormals[i1] += faceNorm;
        outNormals[i2] += faceNorm;
    }
    for (auto& n : outNormals) n = glm::normalize(n);
}

void GetMeshUVs(FbxMesh* mesh, std::vector<glm::vec2>& outUVs)
{
    if (!mesh) return;

    //get all UV set names
    FbxStringList lUVSetNameList;
    mesh->GetUVSetNames(lUVSetNameList);

    //iterating over all uv sets
    for (int lUVSetIndex = 0; lUVSetIndex < lUVSetNameList.GetCount(); lUVSetIndex++)
    {
        //get lUVSetIndex-th uv set
        const char* lUVSetName = lUVSetNameList.GetStringAt(lUVSetIndex);
        const FbxGeometryElementUV* lUVElement = mesh->GetElementUV(lUVSetName);

        if (!lUVElement)
            continue;

        // only support mapping mode eByPolygonVertex and eByControlPoint
        if (lUVElement->GetMappingMode() != FbxGeometryElement::eByPolygonVertex &&
            lUVElement->GetMappingMode() != FbxGeometryElement::eByControlPoint)
            return;

        //index array, where holds the index referenced to the uv data
        const bool lUseIndex = lUVElement->GetReferenceMode() != FbxGeometryElement::eDirect;
        const int lIndexCount = (lUseIndex) ? lUVElement->GetIndexArray().GetCount() : 0;

        //iterating through the data by polygon
        const int lPolyCount = mesh->GetPolygonCount();

        if (lUVElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
        {
            for (int lPolyIndex = 0; lPolyIndex < lPolyCount; ++lPolyIndex)
            {
                // build the max index array that we need to pass into MakePoly
                const int lPolySize = mesh->GetPolygonSize(lPolyIndex);
                for (int lVertIndex = 0; lVertIndex < lPolySize; ++lVertIndex)
                {
                    FbxVector2 lUVValue;

                    //get the index of the current vertex in control points array
                    int lPolyVertIndex = mesh->GetPolygonVertex(lPolyIndex, lVertIndex);

                    //the UV index depends on the reference mode
                    int lUVIndex = lUseIndex ? lUVElement->GetIndexArray().GetAt(lPolyVertIndex) : lPolyVertIndex;

                    lUVValue = lUVElement->GetDirectArray().GetAt(lUVIndex);

                    //User TODO:
                    //Print out the value of UV(lUVValue) or log it to a file
                    outUVs.emplace_back(
                        static_cast<float>(lUVValue[0]),
                        static_cast<float>(lUVValue[1])
                    );
                }
            }
        }
        else if (lUVElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
        {
            int lPolyIndexCounter = 0;
            for (int lPolyIndex = 0; lPolyIndex < lPolyCount; ++lPolyIndex)
            {
                // build the max index array that we need to pass into MakePoly
                const int lPolySize = mesh->GetPolygonSize(lPolyIndex);
                for (int lVertIndex = 0; lVertIndex < lPolySize; ++lVertIndex)
                {
                    if (lPolyIndexCounter < lIndexCount)
                    {
                        FbxVector2 lUVValue;

                        //the UV index depends on the reference mode
                        int lUVIndex = lUseIndex ? lUVElement->GetIndexArray().GetAt(lPolyIndexCounter) : lPolyIndexCounter;

                        lUVValue = lUVElement->GetDirectArray().GetAt(lUVIndex);

                        //User TODO:
                        //Print out the value of UV(lUVValue) or log it to a file
                        outUVs.emplace_back(
                            static_cast<float>(lUVValue[0]),
                            static_cast<float>(lUVValue[1])
                        );

                        lPolyIndexCounter++;
                    }
                }
            }
        }
    }
}

glm::mat4 fbxToGlm(const FbxAMatrix& m) {
    glm::mat4 out;

    // For each row r and column c of the FBX matrix,
    // assign to out[c][r] because glm is column-major.
    for (int r = 0; r < 4; ++r) {
        FbxVector4 row = m.GetRow(r);
        out[0][r] = static_cast<float>(row[0]);
        out[1][r] = static_cast<float>(row[1]);
        out[2][r] = static_cast<float>(row[2]);
        out[3][r] = static_cast<float>(row[3]);
    }

    return out;
}