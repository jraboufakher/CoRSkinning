#include "FBXLoader.h"
#include <iostream>
#include <map>
#include <algorithm>
#include <iomanip>

FBXLoader::FBXLoader() {
    InitializeSdkObjects(pManager, pScene);
}

FBXLoader::~FBXLoader() {
    DestroySdkObjects(pManager, pExitStatus);
}

glm::mat4 FBXLoader::fbxToGlm(const FbxAMatrix& m)
{
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

void FBXLoader::InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene)
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

void FBXLoader::DestroySdkObjects(FbxManager* pManager, bool pExitStatus)
{
    //Delete the FBX Manager. All the objects that have been allocated using the FBX Manager and that haven't been explicitly destroyed are also automatically destroyed.
    if (pManager) pManager->Destroy();
    if (pExitStatus) FBXSDK_printf("Program Success!\n");
}

bool FBXLoader::LoadScene(const char* pFilename)
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

        // Trinagulate Scene
        triangulateScene();

        // Extract Data
        FbxNode* rootNode = pScene->GetRootNode();
        if (rootNode) {
            extractSkeletonData();
            extractMeshData(rootNode);
        }
    }
    // Destroy the importer.
    lImporter->Destroy();

    return lStatus;
}

void FBXLoader::triangulateScene()
{
    FbxGeometryConverter converter(pManager);
    converter.Triangulate(pScene, true);
}

void FBXLoader::extractMeshData(FbxNode* node)
{
    if (!node) return;

    FbxNodeAttribute* attr = node->GetNodeAttribute();
    if (attr && attr->GetAttributeType() == FbxNodeAttribute::eMesh)
    {
        FbxMesh* mesh = node->GetMesh();

        // Vertices 
        int controlPointCount = mesh->GetControlPointsCount();
        FbxVector4* controlPoints = mesh->GetControlPoints();
        mesh_.vertices.reserve(controlPointCount);
        for (int i = 0; i < controlPointCount; ++i) {
            FbxVector4& p = controlPoints[i];
            glm::vec3 vertex(static_cast<float>(p[0]),
                static_cast<float>(p[1]),
                static_cast<float>(p[2]));
            mesh_.vertices.push_back(vertex);
        }

        // Faces
        int polygonCount = mesh->GetPolygonCount();
        mesh_.faces.reserve(polygonCount * 3);

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
                    mesh_.faces.push_back(static_cast<unsigned int>(controlPointIndex));
                }
            }
            // For polygons with more than 3 vertices, use a triangle fan to triangulate.
            else {
                int v0 = mesh->GetPolygonVertex(polygonIndex, 0);
                for (int vertexIndex = 1; vertexIndex < vertexCount - 1; ++vertexIndex) {
                    int v1 = mesh->GetPolygonVertex(polygonIndex, vertexIndex);
                    int v2 = mesh->GetPolygonVertex(polygonIndex, vertexIndex + 1);
                    mesh_.faces.push_back(static_cast<unsigned int>(v0));
                    mesh_.faces.push_back(static_cast<unsigned int>(v1));
                    mesh_.faces.push_back(static_cast<unsigned int>(v2));
                }
            }
        }

        // Weights and Skeleton Data
        getBoneData(mesh);

        // Normals and UVs
        computeNormals(mesh);
        computeUVs(mesh);

        return;
    }
    for (int i = 0; i < node->GetChildCount(); ++i)
    {
        FBXLoader::extractMeshData(node->GetChild(i));
    }
}

void FBXLoader::extractSkeletonData()
{
    skeleton_.bones.clear();
    skeleton_.numberOfBones = 0;
    extractSkeletonRecursive(pScene->GetRootNode(), -1);    // the root bone will have an index of -1
}

void FBXLoader::extractSkeletonRecursive(FbxNode* node, int parentIndex)
{
    if (!node) return;

    int currentIndex = parentIndex;

    FbxNodeAttribute* attr = node->GetNodeAttribute();
    if (attr && attr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
    {
        Bone bone;
        bone.node = node;
        bone.parentIndex = parentIndex;

        // Compute bind pose inverse at time t0
        FbxTime t0;
        t0.SetSecondDouble(0.0);
        FbxAMatrix mat = node->EvaluateGlobalTransform(t0);
        bone.bindPoseInverse = glm::inverse(fbxToGlm(mat));

        // Add the bone to the skeleton
        currentIndex = static_cast<int>(skeleton_.bones.size());
        skeleton_.bones.push_back(bone);
        skeleton_.numberOfBones++;
    }

    // Recursively go through all the bones.
    for (int i = 0; i < node->GetChildCount(); ++i) {
        extractSkeletonRecursive(node->GetChild(i), currentIndex);
    }
}

void FBXLoader::getBoneData(FbxMesh* mesh)
{
    if (!mesh) return;

    int controlPointCount = mesh->GetControlPointsCount();
    std::map<int, std::vector<BoneInfluence>> skinWeights;

    int skinCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
    if (skinCount == 0)
        return;

    // Influences per control point: for each skin-deformer and each cluster
    for (int i = 0; i < skinCount; ++i) {
        auto* skin = static_cast<FbxSkin*>(mesh->GetDeformer(i, FbxDeformer::eSkin));
        int clusterCount = skin->GetClusterCount();

        for (int j = 0; j < clusterCount; ++j) {
            auto* cluster = skin->GetCluster(j);
            FbxNode* linkNode = cluster->GetLink();
            // find that linkNode in skeleton_.boneNodes
            int boneIndex = -1;
            for (int b = 0; b < (int)skeleton_.bones.size(); ++b) {
                if (skeleton_.bones[b].node == linkNode) {
                    boneIndex = b;
                    break;
                }
            }
            if (boneIndex < 0) {
                std::cerr << "Warning: bone '" << linkNode->GetName() << "' not found in skeleton.\n";
                continue;
            }
            // apply that boneIndex to its vertex
            int* indices = cluster->GetControlPointIndices();
            double* weights = cluster->GetControlPointWeights();
            int count = cluster->GetControlPointIndicesCount();
            for (int k = 0; k < count; k++) {
                int cpIndex = indices[k];
                float w = static_cast<float>(weights[k]);
                if (w > 0.0f) {
                    skinWeights[cpIndex].push_back({ boneIndex, w });
                }
            }
        }
    }

    mesh_.boneIndices.resize(controlPointCount);
    mesh_.boneWeights.resize(controlPointCount);

    // Normalize and fill output
    for (int cpIndex = 0; cpIndex < controlPointCount; ++cpIndex) {
        auto& influences = skinWeights[cpIndex];
        std::sort(influences.begin(), influences.end(), [](const BoneInfluence& a, const BoneInfluence& b) { return a.weight > b.weight; });

        float total = 0.0f;
        for (const auto& inf : influences)
            total += inf.weight;

        if (total > 0.0f) {
            for (const auto& inf : influences) {
                mesh_.boneIndices[cpIndex].push_back(static_cast<unsigned int>(inf.boneIndex));
                mesh_.boneWeights[cpIndex].push_back(inf.weight / total);
            }
        }
    }
}

void FBXLoader::computeNormals(FbxMesh* mesh)
{
    if (!mesh) return;
    int polyCount = mesh->GetPolygonCount();
    mesh_.normals.resize(mesh->GetControlPointsCount(), glm::vec3(0));
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
        mesh_.normals[i0] += faceNorm;
        mesh_.normals[i1] += faceNorm;
        mesh_.normals[i2] += faceNorm;
    }
    for (auto& n : mesh_.normals) n = glm::normalize(n);
}

void FBXLoader::computeUVs(FbxMesh* mesh)
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

                    mesh_.uvs.emplace_back(
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

                        mesh_.uvs.emplace_back(
                            glm::vec2(static_cast<float>(lUVValue[0]),
                                static_cast<float>(lUVValue[1]))
                        );

                        lPolyIndexCounter++;
                    }
                }
            }
        }
    }
}


