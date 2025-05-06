#include "render/Mesh.h"
#include <iostream>
#include <cstring>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <unordered_map>

void Mesh::initBuffers() {
    // Generate & bind a VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Positions @ layout(0)
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

    // Normals @ layout(1)
    glGenBuffers(1, &vboNorm);
    glBindBuffer(GL_ARRAY_BUFFER, vboNorm);
    glBufferData(GL_ARRAY_BUFFER,
        normals.size() * sizeof(glm::vec3),
        normals.data(),
        GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
        sizeof(glm::vec3), (void*)0);

    // UVs @ layout(2) (if present)
    if (!uvs.empty()) {
        glGenBuffers(1, &vboUV);
        glBindBuffer(GL_ARRAY_BUFFER, vboUV);

        //std::cout << "[DEBUG] uvs.size() = " << uvs.size() << ", first UV = (" << uvs[0].x << "," << uvs[0].y << ")\n";

        glBufferData(GL_ARRAY_BUFFER,
            uvs.size() * sizeof(glm::vec2),
            uvs.data(),
            GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
            sizeof(glm::vec2), (void*)0);

        GLint enabled = 0;
        glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
        //std::cout << "[DEBUG] attrib 2 enabled = " << enabled << "\n";
    }

    // CoRs @ layout(6)
    glGenBuffers(1, &vboCoR);
    glBindBuffer(GL_ARRAY_BUFFER, vboCoR);
    glBufferData(GL_ARRAY_BUFFER,
        centersOfRotation.size() * sizeof(glm::vec3),
        centersOfRotation.data(),
        GL_STATIC_DRAW);
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE,
        sizeof(glm::vec3), (void*)0);

    // Pack skinInfo into a temporary VBO
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

    // Bone IDs @ layout(4) ← integer attribute
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(
        /*index*/   4,
        /*size*/    4,
        /*type*/    GL_INT,
        /*stride*/  sizeof(SkinPack),
        /*offset*/  (void*)offsetof(SkinPack, id)
    );

    // Bone Weights @ layout(5)
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(
        /*index*/   5,
        /*size*/    4,
        /*type*/    GL_FLOAT,
        /*normalized*/ GL_FALSE,
        /*stride*/  sizeof(SkinPack),
        /*offset*/  (void*)offsetof(SkinPack, w)
    );

    // Element array (indices)
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        std::cerr << "GL error after buffer setup: " << err << "\n";

    // Unbind VAO
    glBindVertexArray(0);
}

void Mesh::draw() const {
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES,
        GLsizei(indices.size()),
        GL_UNSIGNED_INT, 0);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        std::cerr << "GL error after draw: " << err << "\n";
    
    glBindVertexArray(0);
}

void Mesh::uploadSkeletonUniforms(GLuint skinProg, const std::vector<glm::mat4>& boneMatrices, const std::vector<DualQuaternion>& boneDualQuats)
{
    GLsizei numBones = (GLsizei)boneMatrices.size();
    if (boneDualQuats.size() != boneMatrices.size()) {
        std::cerr << "uploadSkeletonUniforms: size mismatch\n";
        return;
    }

    // skeleton.numBones
    GLint locNum = glGetUniformLocation(skinProg, "skeleton.numBones");
    glUniform1i(locNum, numBones);

    // for each bone, upload .transform, .dqTransform.real, .dqTransform.dual
    for (GLsizei i = 0; i < numBones; ++i) {
        // a) 4x4 matrix
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

void Mesh::flattenVertices()
{
    std::vector<glm::vec3> newPos;
    std::vector<glm::vec3> newNorm;
    std::vector<glm::vec2> newUV;
    std::vector<unsigned int> newIdx;
    std::vector<glm::vec3> newCoR;
    std::vector<VertexSkinData> newSkin;

    newPos.reserve(indices.size());
    newNorm.reserve(indices.size());
    newUV.reserve(indices.size());
    newIdx.reserve(indices.size());
    newCoR.reserve(indices.size());
    newSkin.reserve(indices.size());

    std::unordered_map<VertexKey, unsigned int, VertexKeyHash> seen;
    seen.reserve(indices.size());

    for (size_t i = 0; i < indices.size(); ++i) {
        VertexKey key{
            /*posIdx*/ static_cast<int>(indices[i]),
            /*normIdx*/ static_cast<int>(indices[i]),
            /*uvIdx*/   static_cast<int>(i)
        };

        auto it = seen.find(key);
        if (it == seen.end()) {
            unsigned int newIndex = static_cast<unsigned int>(newPos.size());
            seen[key] = newIndex;

            // copy all the per‐vertex data from the old arrays
            newPos.push_back(positions[key.posIdx]);
            newNorm.push_back(normals[key.normIdx]);
            newUV.push_back(uvs[key.uvIdx]);
            newCoR.push_back(centersOfRotation[key.posIdx]);
            newSkin.push_back(skinInfo[key.posIdx]);

            newIdx.push_back(newIndex);
        }
        else {
            newIdx.push_back(it->second);
        }
    }

    // swap everything into place
    positions.swap(newPos);
    normals.swap(newNorm);
    uvs.swap(newUV);
    indices.swap(newIdx);
    centersOfRotation.swap(newCoR);
    skinInfo.swap(newSkin);
}

DualQuaternion makeDualQuat(const glm::mat4& M)
{
    // Extract rotation quaternion from upper‐left 3×3
    glm::quat q = glm::quat_cast(M);

    // Extract translation vector from the 4th column
    glm::vec3 t = glm::vec3(M[3]);

    // Build "translation quaternion" qt = (0,  t)
    glm::quat qt(0, t.x, t.y, t.z);

    // Dual part
    glm::quat dq = 0.5f * (qt * q);

    DualQuaternion out;
    out.real = glm::vec4(q.x, q.y, q.z, q.w);
    out.dual = glm::vec4(dq.x, dq.y, dq.z, dq.w);
    return out;
}
