﻿#include "cor/Mesh.h"
#include <iostream>
#include <string>
#include <cstring>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

void Mesh::initBuffers() {
    // 1) Generate & bind a VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // 2) Positions @ layout(0)
    glGenBuffers(1, &vboPos);
    glBindBuffer(GL_ARRAY_BUFFER, vboPos);
    glBufferData(GL_ARRAY_BUFFER,
        positions.size() * sizeof(glm::vec3),
        positions.data(),
        GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        /*index*/     0,
        /*size*/      3,
        /*type*/      GL_FLOAT,
        /*normalized*/GL_FALSE,
        /*stride*/    sizeof(glm::vec3),
        /*offset*/    (void*)0
    );

    // 3) Normals @ layout(1)
    glGenBuffers(1, &vboNorm);
    glBindBuffer(GL_ARRAY_BUFFER, vboNorm);
    glBufferData(GL_ARRAY_BUFFER,
        normals.size() * sizeof(glm::vec3),
        normals.data(),
        GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
        sizeof(glm::vec3), (void*)0);

    // 4) UVs @ layout(2) (if present)
    if (!uvs.empty()) {
        glGenBuffers(1, &vboUV);
        glBindBuffer(GL_ARRAY_BUFFER, vboUV);
        glBufferData(GL_ARRAY_BUFFER,
            uvs.size() * sizeof(glm::vec2),
            uvs.data(),
            GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
            sizeof(glm::vec2), (void*)0);
    }

    // 5) CoRs @ layout(6)
    glGenBuffers(1, &vboCoR);
    glBindBuffer(GL_ARRAY_BUFFER, vboCoR);
    glBufferData(GL_ARRAY_BUFFER,
        centersOfRotation.size() * sizeof(glm::vec3),
        centersOfRotation.data(),
        GL_STATIC_DRAW);
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE,
        sizeof(glm::vec3), (void*)0);

    // 6) Pack skinInfo into a temporary VBO
    struct SkinPack {
        int   id[4];   // integer bone IDs
        float w[4];   // float bone weights
    };
    std::vector<SkinPack> pack(skinInfo.size());
    for (size_t i = 0; i < skinInfo.size(); ++i) {
        for (int j = 0; j < 4; ++j) {
            pack[i].id[j] = skinInfo[i].boneIDs[j];
            pack[i].w[j] = skinInfo[i].weights[j];
        }
    }

    glGenBuffers(1, &vboSkin);
    glBindBuffer(GL_ARRAY_BUFFER, vboSkin);
    glBufferData(GL_ARRAY_BUFFER,
        pack.size() * sizeof(SkinPack),
        pack.data(),
        GL_STATIC_DRAW);

    // 6a) Bone IDs @ layout(4) ← integer attribute
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(
        /*index*/   4,
        /*size*/    4,
        /*type*/    GL_INT,
        /*stride*/  sizeof(SkinPack),
        /*offset*/  (void*)offsetof(SkinPack, id)
    );

    // 6b) Weights @ layout(5)
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(
        /*index*/   5,
        /*size*/    4,
        /*type*/    GL_FLOAT,
        /*normalized*/ GL_FALSE,
        /*stride*/  sizeof(SkinPack),
        /*offset*/  (void*)offsetof(SkinPack, w)
    );

    // 7) Element array (indices)
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW);

    // 8) Done: unbind VAO
    glBindVertexArray(0);
}

void Mesh::draw() const {
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES,
        GLsizei(indices.size()),
        GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::uploadSkeletonUniforms(GLuint skinProg,
    const std::vector<glm::mat4>& boneMatrices,
    const std::vector<DualQuaternion>& boneDualQuats)
{
    GLsizei numBones = (GLsizei)boneMatrices.size();
    if (boneDualQuats.size() != boneMatrices.size()) {
        std::cerr << "uploadSkeletonUniforms: size mismatch\n";
        return;
    }

    // 1) skeleton.numBones
    GLint locNum = glGetUniformLocation(skinProg, "skeleton.numBones");
    glUniform1i(locNum, numBones);

    // 2) for each bone, upload .transform, .dqTransform.real, .dqTransform.dual
    for (GLsizei i = 0; i < numBones; ++i) {
        // a) classic 4x4 matrix
        {
            std::string name = "skeleton.bone[" + std::to_string(i) + "].transform";
            GLint loc = glGetUniformLocation(skinProg, name.c_str());
            glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(boneMatrices[i]));
        }

        // b) dual‐quaternion real part
        {
            std::string name = "skeleton.bone[" + std::to_string(i) + "].dqTransform.real";
            GLint loc = glGetUniformLocation(skinProg, name.c_str());
            glUniform4fv(loc, 1, glm::value_ptr(boneDualQuats[i].real));
        }

        // c) dual‐quaternion dual part
        {
            std::string name = "skeleton.bone[" + std::to_string(i) + "].dqTransform.dual";
            GLint loc = glGetUniformLocation(skinProg, name.c_str());
            glUniform4fv(loc, 1, glm::value_ptr(boneDualQuats[i].dual));
        }
    }
}

DualQuaternion makeDualQuat(const glm::mat4& M)
{
    // 1) Extract rotation quaternion from upper‐left 3×3
    glm::quat q = glm::quat_cast(M);

    // 2) Extract translation vector from the 4th column
    glm::vec3 t = glm::vec3(M[3]);

    // 3) Build "translation quaternion" qt = (0,  t)
    glm::quat qt(0, t.x, t.y, t.z);

    // 4) Dual part = 0.5 * (qt * q)
    glm::quat dq = 0.5f * (qt * q);

    DualQuaternion out;
    out.real = glm::vec4(q.x, q.y, q.z, q.w);
    out.dual = glm::vec4(dq.x, dq.y, dq.z, dq.w);
    return out;
}
