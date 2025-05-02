#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>

static const int MAX_INFLUENCES = 4;
struct VertexSkinData {
    int   boneIDs[MAX_INFLUENCES];
    float weights [MAX_INFLUENCES];
};

struct DualQuaternion {
    glm::vec4 real;
    glm::vec4 dual;
};
DualQuaternion makeDualQuat(const glm::mat4& M);

struct SkeletonBone {
    glm::vec3 pos;
    glm::mat4 transform;
    DualQuaternion dqTransform;
};

class Mesh {
public:
    // CPU data
    std::vector<glm::vec3> positions, normals, centersOfRotation;
    std::vector<glm::vec2> uvs;              // if you have them
    std::vector<unsigned int> indices;
    std::vector<VertexSkinData> skinInfo;

    // GPU handles
    GLuint vao;
    GLuint vboPos, vboNorm, vboUV, ebo;
    GLuint vboCoR, vboSkin; // one VBO for boneIDs+weights

    void initBuffers();
    void draw() const;

    void uploadSkeletonUniforms(GLuint skinProg,
        const std::vector<glm::mat4>& boneMatrices,
        const std::vector<DualQuaternion>& boneDualQuats);
};
