#version 430
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

// We’ll pack accumulators as uvec4: x = sumW_fixed, yzw = sumW*pos_fixed
struct GpuVertex {
    vec3  pos;
    uvec4 boneIds;
    vec4  boneWts;
};

layout(std430, binding = 0) readonly buffer Verts {
    GpuVertex verts[];
};

layout(std430, binding = 1) buffer Accum {
    uvec4 accum[];   // integer accumulators
};

// Choose a scale: how many fractional steps per unit
const uint SCALE = 10000u;

void main() {
    uint vid = gl_GlobalInvocationID.x;
    if (vid >= verts.length()) return;

    GpuVertex v = verts[vid];
    for (int k = 0; k < 4; ++k) {
        uint b = v.boneIds[k];
        float w  = v.boneWts[k];
        if (w == 0.0) continue;

        // convert to fixed-point
        uint wf = uint(w * SCALE + 0.5);
        atomicAdd(accum[b].x, wf);

        // weighted positions, also fixed
        atomicAdd(accum[b].y, uint((v.pos.x * w) * SCALE + 0.5));
        atomicAdd(accum[b].z, uint((v.pos.y * w) * SCALE + 0.5));
        atomicAdd(accum[b].w, uint((v.pos.z * w) * SCALE + 0.5));
    }
}
