#version 330 core

#define MAX_BONES 100

// vertex streams
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 4) in ivec4 aBoneIDs;    // integer bone‐indices
layout(location = 5) in vec4 aWeights;     // bone‐weights
layout(location = 6) in vec3 aCoR;         // precomputed center-of-rotation

// common uniforms
uniform mat4 uView;
uniform mat4 uProj;

// send in your per-bone matrices and quaternions
uniform mat4 uBoneMatrices[MAX_BONES];
uniform vec4 uBoneQuaternions[MAX_BONES];

// simple Phong-ish output
out vec3 vNormal;

// helper: turn a (x,y,z,w) quaternion into a 3×3 rotation
mat3 quatToMat3(vec4 q) {
    float x = q.x, y = q.y, z = q.z, w = q.w;
    float x2 = x + x, y2 = y + y, z2 = z + z;
    float xx = x * x2, yy = y * y2, zz = z * z2;
    float xy = x * y2, xz = x * z2, yz = y * z2;
    float wx = w * x2, wy = w * y2, wz = w * z2;
    return mat3(
        1.0 - (yy + zz), xy + wz,       xz - wy,
        xy - wz,       1.0 - (xx + zz), yz + wx,
        xz + wy,       yz - wx,        1.0 - (xx + yy)
    );
}

void main() {
    // 1) Blend the quaternions (QLERP)
    vec4 qSum = vec4(0.0);
    for(int i = 0; i < 4; ++i) {
        int b = aBoneIDs[i];
        qSum += aWeights[i] * uBoneQuaternions[b];
    }
    qSum = normalize(qSum);
    mat3 R = quatToMat3(qSum);

    // 2) Compute LBS matrix at the CoR
    mat4 Mlb = mat4(0.0);
    for(int i = 0; i < 4; ++i) {
        Mlb += aWeights[i] * uBoneMatrices[aBoneIDs[i]];
    }

    // 3) Figure out the translation offset t = LBS(aCoR) – R·aCoR
    vec3 worldCoR = (Mlb * vec4(aCoR, 1.0)).xyz;
    vec3 t       = worldCoR - R * aCoR;

    // 4) Do the CRS transform: rotate about aCoR, then translate
    vec3 localPos = aPos - aCoR;
    vec3 skinned  = R * localPos + aCoR + t;

    // 5) Finish vertex
    gl_Position = uProj * uView * vec4(skinned, 1.0);
    vNormal     = R * aNorm; 
}
