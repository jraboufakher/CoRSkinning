#version 330 core

//-----------------------------------------------------------------------------
// quaternion.glsl (only what's used by CRS)
//-----------------------------------------------------------------------------

// Oriented addition of weighted quaternions
vec4 quat_add_oriented(vec4 q1, vec4 q2)
{
    // If the dot is negative, flip the second quaternion so they add “in phase”
    if (dot(q1, q2) >= 0.0) {
        return q1 + q2;
    } else {
        return q1 - q2;
    }
}

// Convert a unit quaternion to a rotation matrix
mat3 quat_toRotationMatrix(vec4 quat)
{
    float twiceXY  = 2.0 * quat.x * quat.y;
    float twiceXZ  = 2.0 * quat.x * quat.z;
    float twiceYZ  = 2.0 * quat.y * quat.z;
    float twiceWX  = 2.0 * quat.w * quat.x;
    float twiceWY  = 2.0 * quat.w * quat.y;
    float twiceWZ  = 2.0 * quat.w * quat.z;

    float wSqrd = quat.w * quat.w;
    float xSqrd = quat.x * quat.x;
    float ySqrd = quat.y * quat.y;
    float zSqrd = quat.z * quat.z;

    return mat3(
        // column 0
        vec3(
            1.0 - 2.0 * (ySqrd + zSqrd),
            twiceXY + twiceWZ,
            twiceXZ - twiceWY
        ),
        // column 1
        vec3(
            twiceXY - twiceWZ,
            1.0 - 2.0 * (xSqrd + zSqrd),
            twiceYZ + twiceWX
        ),
        // column 2
        vec3(
            twiceXZ + twiceWY,
            twiceYZ - twiceWX,
            1.0 - 2.0 * (xSqrd + ySqrd)
        )
    );
}


//-----------------------------------------------------------------------------
// dualquaternion.glsl (only the struct, since CRS reads dqTransform.real)
//-----------------------------------------------------------------------------

struct DualQuaternion {
    vec4 real;
    vec4 dual;
};

//-----------------------------------------------------------------------------
// skeletons.glsl (only CRS branch, with “centerOfRotation”)
//-----------------------------------------------------------------------------


#define MAX_NUM_BONES_PER_MESH 100

layout (location = 4) in ivec4 SkeletonBoneIndices;
layout (location = 5) in vec4 SkeletonBoneWeights;
layout (location = 6) in vec3 centerOfRotation;

struct SkeletonBone {
    vec3            pos;
    mat4            transform;
    DualQuaternion  dqTransform;
};

struct Skeleton {
    SkeletonBone bone[MAX_NUM_BONES_PER_MESH];
    int           numBones;
};

uniform Skeleton skeleton;

#ifdef SKELETAL_ANIMATION_CRS_OUT
out vec3 cor;
#endif

//-----------------------------------------------------------------------------
// Vertex streams
//-----------------------------------------------------------------------------

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aUV;

// Common uniforms
uniform mat4 uView;
uniform mat4 uProj;

// Phong-ish output
out vec3 vNormal;
out vec2 vUV;

//-----------------------------------------------------------------------------
// CRS-only skinning function
//-----------------------------------------------------------------------------

mat4 perform_skinning_crs()
{
    vec4 quatRotation = vec4(0.0);
    mat4 lbs          = mat4(0.0);

    for (int i = 0; i < 4; ++i) {
        int   idx    = int(SkeletonBoneIndices[i]);
        float weight = SkeletonBoneWeights[i];

        vec4 bq = skeleton.bone[idx].dqTransform.real;
        quatRotation = quat_add_oriented(quatRotation, weight * bq);
        lbs += weight * skeleton.bone[idx].transform;
    }

    quatRotation = normalize(quatRotation);
    mat3 R        = quat_toRotationMatrix(quatRotation);

    vec3 corLBS = (lbs * vec4(centerOfRotation, 1.0)).xyz;
    vec3 corRot = R * centerOfRotation;
    vec4 trans  = vec4(corLBS - corRot, 1.0);

    mat4 skinMat = mat4(R);
    skinMat[3]   = trans;

    return skinMat;
}

//-----------------------------------------------------------------------------
// Main vertex shader
//-----------------------------------------------------------------------------

void main()
{
    mat4 skinMat = perform_skinning_crs();

    // Position
    vec4 skPos = skinMat * vec4(aPos, 1.0);
    gl_Position = uProj * uView * skPos;

    // Normal (in view space)
    vNormal = mat3(uView * skinMat) * aNorm;

    // UV
    vUV = aUV;
}
