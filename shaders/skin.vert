#version 330 core

#ifndef quaternion_glsl
#define quaternion_glsl

float quat_norm(in vec4 a)
{
	return length(a);
}

vec4 quat_conj(vec4 q)
{ 
  return vec4(-q.xyz, q.w); 
}
  
vec4 quat_mult(vec4 q1, vec4 q2)
{ 
  vec4 qr;
  qr.x = (q1.w * q2.x) + (q1.x * q2.w) + (q1.y * q2.z) - (q1.z * q2.y);
  qr.y = (q1.w * q2.y) - (q1.x * q2.z) + (q1.y * q2.w) + (q1.z * q2.x);
  qr.z = (q1.w * q2.z) + (q1.x * q2.y) - (q1.y * q2.x) + (q1.z * q2.w);
  qr.w = (q1.w * q2.w) - (q1.x * q2.x) - (q1.y * q2.y) - (q1.z * q2.z);
  return qr;
}

// function from https://twistedpairdevelopment.wordpress.com/2013/02/11/rotating-a-vector-by-a-quaternion-in-glsl/
vec4 multQuat(vec4 q1, vec4 q2)
{
	return vec4(
	q1.w * q2.x + q1.x * q2.w + q1.z * q2.y - q1.y * q2.z,
	q1.w * q2.y + q1.y * q2.w + q1.x * q2.z - q1.z * q2.x,
	q1.w * q2.z + q1.z * q2.w + q1.y * q2.x - q1.x * q2.y,
	q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z
	);
}

vec4 quat_add_oriented(vec4 q1, vec4 q2)
{
	if (dot(q1, q2) >= 0) {
		return q1 + q2;
	} else {
		return q1 - q2;
	}
}

mat3 quat_toRotationMatrix(vec4 quat)
{
	float twiceXY  = 2 * quat.x * quat.y;
	float twiceXZ  = 2 * quat.x * quat.z;
	float twiceYZ  = 2 * quat.y * quat.z;
	float twiceWX  = 2 * quat.w * quat.x;
	float twiceWY  = 2 * quat.w * quat.y;
	float twiceWZ  = 2 * quat.w * quat.z;

	float wSqrd = quat.w * quat.w;
	float xSqrd = quat.x * quat.x;
	float ySqrd = quat.y * quat.y;
	float zSqrd = quat.z * quat.z;

	return mat3(
		// first column
		vec3(
			1.0f - 2*(ySqrd + zSqrd),
			twiceXY + twiceWZ,
			twiceXZ - twiceWY),

		// second column
		vec3(
			twiceXY - twiceWZ,
			1.0f - 2*(xSqrd + zSqrd),
			twiceYZ + twiceWX),

		// third column
		vec3(
			twiceXZ + twiceWY,
			twiceYZ - twiceWX,
			1.0f - 2*(xSqrd + ySqrd))
	);
}

vec4 quat_from_axis_angle(vec3 axis, float angle)
{ 
  vec4 qr;
  float half_angle = (angle * 0.5) * 3.14159 / 180.0;
  qr.x = axis.x * sin(half_angle);
  qr.y = axis.y * sin(half_angle);
  qr.z = axis.z * sin(half_angle);
  qr.w = cos(half_angle);
  return qr;
}

vec4 quat_from_axis_angle_rad(vec3 axis, float angle)
{ 
  vec4 qr;
  float half_angle = (angle * 0.5);
  qr.x = axis.x * sin(half_angle);
  qr.y = axis.y * sin(half_angle);
  qr.z = axis.z * sin(half_angle);
  qr.w = cos(half_angle);
  return qr;
}

vec3 rotate_vertex_position(vec3 position, vec3 axis, float angle)
{ 
  vec4 qr = quat_from_axis_angle(axis, angle);
  vec4 qr_conj = quat_conj(qr);
  vec4 q_pos = vec4(position.x, position.y, position.z, 0);
  
  vec4 q_tmp = quat_mult(qr, q_pos);
  qr = quat_mult(q_tmp, qr_conj);
  
  return vec3(qr.x, qr.y, qr.z);
}

// function from https://twistedpairdevelopment.wordpress.com/2013/02/11/rotating-a-vector-by-a-quaternion-in-glsl/
vec3 rotate_vector( vec4 quat, vec3 vec )
{
	vec4 qv = multQuat( quat, vec4(vec, 0.0) );
	return multQuat( qv, vec4(-quat.x, -quat.y, -quat.z, quat.w) ).xyz;
}
// function from https://twistedpairdevelopment.wordpress.com/2013/02/11/rotating-a-vector-by-a-quaternion-in-glsl/
vec3 rotate_vector_optimized( vec4 quat, vec3 vec )
{
	return vec + 2.0 * cross( cross( vec, quat.xyz ) + quat.w * vec, quat.xyz );
}

vec4 quat_from_two_vectors(vec3 u, vec3 v)
{
	vec3 unorm = normalize(u);
	vec3 vnorm = normalize(v);
    float cos_theta = dot(unorm, vnorm);
    float angle = acos(cos_theta);
    vec3 w = normalize(cross(u, v));
    return quat_from_axis_angle_rad(w, angle);
}

#endif

#ifndef dualquaternion_glsl
#define dualquaternion_glsl

#define DUALQUATERNION_EPSILON 0.1

// real part + dual part
// = real + e*dual
// = q1   + e*q2
// = (w0 + i*x0 + j*y0 + k*z0)   +   e*(we + i*xe + j*ye + k*ze)
struct DualQuaternion {
	vec4 real, dual;
};

vec3 dualquat_getTranslation(in DualQuaternion dq)
{
	return (2 * quat_mult(dq.dual, quat_conj(dq.real))).xyz;
}

DualQuaternion dualquat_conj(in DualQuaternion dq)
{
	return DualQuaternion(quat_conj(dq.real), quat_conj(dq.dual));
}

float dualquat_norm(in DualQuaternion dq)
{
	return quat_norm(dq.real);
}

DualQuaternion dualquat_add(in DualQuaternion first, in DualQuaternion second)
{
	if (dot(first.real, second.real) >= 0) {
		return DualQuaternion(first.real + second.real, first.dual + second.dual);
	} else {
		return DualQuaternion(first.real - second.real, first.dual - second.dual);
	}
}

DualQuaternion dualquat_mult(in float scalar, in DualQuaternion dq)
{
	return DualQuaternion(scalar * dq.real, scalar * dq.dual);
}

DualQuaternion dualquat_mult(in DualQuaternion first, in DualQuaternion second)
{
	return DualQuaternion(
		quat_mult(first.real, second.real),
		quat_mult(first.real, second.dual) + quat_mult(first.dual, second.real)
		);
}

DualQuaternion dualquat_normalize(in DualQuaternion dq)
{
	float inv_real_norm = 1.0 / quat_norm(dq.real);
	return DualQuaternion(
		dq.real * inv_real_norm,
		dq.dual * inv_real_norm);
}

#endif //dualquaternion_glsl


// skeleton.glsl
#define SKELETAL_ANIMATION_MODE_LBS 0
#define SKELETAL_ANIMATION_MODE_DQS 1
#define SKELETAL_ANIMATION_MODE_CRS 2

#define MAX_NUM_BONES_PER_MESH 72

layout (location = 4) in vec4 SkeletonBoneIndices;
layout (location = 5) in vec4 SkeletonBoneWeights;
layout (location = 6) in vec3 centerOfRotation;

struct SkeletonBone {
	vec3 pos;
	mat4 transform;
	DualQuaternion dqTransform;
};

struct Skeleton {
	SkeletonBone bone[MAX_NUM_BONES_PER_MESH];
	int numBones;
};

 // LBS is used, if no other technique is selected
uniform int SkinningMode = SKELETAL_ANIMATION_MODE_LBS;
uniform Skeleton skeleton;

#ifdef SKELETAL_ANIMATION_CRS_OUT
out vec3 cor;
#endif

mat4 perform_skinning()
{

	//if (World_Mesh.SkeletonIndex < 0)
	//	return mat4(1.0);

	mat4 result = mat4(0);

#ifdef SKELETAL_ANIMATION_CRS_OUT
	cor = centerOfRotation;
#endif

	if (SkinningMode == SKELETAL_ANIMATION_MODE_DQS) {
		DualQuaternion dqResult = DualQuaternion(vec4(0,0,0,0), vec4(0));
		for (int i = 0; i < 4; ++i) {
			dqResult = dualquat_add(dqResult, dualquat_mult(SkeletonBoneWeights[i], skeleton.bone[int(SkeletonBoneIndices[i])].dqTransform));
			//dqResult = dualquat_normalize(dqResult);
		}

		DualQuaternion c = dualquat_normalize(dqResult);
		vec4 translation = vec4(dualquat_getTranslation(c), 1);
		result = mat4(quat_toRotationMatrix(c.real));
		result[3] = translation;
	} else if (SkinningMode == SKELETAL_ANIMATION_MODE_CRS) {
		vec4 quatRotation = vec4(0);
		mat4 lbs = mat4(0);

		for (int i = 0; i < 4; ++i) {
			quatRotation = quat_add_oriented(quatRotation, SkeletonBoneWeights[i] * skeleton.bone[int(SkeletonBoneIndices[i])].dqTransform.real);
			lbs += SkeletonBoneWeights[i] * skeleton.bone[int(SkeletonBoneIndices[i])].transform;
		}
	
		quatRotation = normalize(quatRotation);
		mat3 quatRotationMatrix = quat_toRotationMatrix(quatRotation);

		vec4 translation = vec4((lbs*vec4(centerOfRotation, 1.0) - vec4(quatRotationMatrix*centerOfRotation, 0.0)).xyz, 1.0);

		result = mat4(quatRotationMatrix);
		result[3] = translation;
	} else { // SKELETAL_ANIMATION_MODE_LBS
		for (int i = 0; i < 4; ++i) {
			result += SkeletonBoneWeights[i] * skeleton.bone[int(SkeletonBoneIndices[i])].transform;
		}
	}

	return result;
}

// skeleton.glsl

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNorm;
layout(location=2) in vec2 aUV;

uniform mat4 uProjView;
out vec2 vUV;
out vec3 vNormal;

void main(){
    // tell the shader to use CoR‐skinning
    // (set from C++: SkinningMode=2)
    mat4 skin = perform_skinning();

    vec4 world = skin * vec4(aPos,1.0);
    gl_Position = uProjView * world;

    // propagate varyings
    vUV     = aUV;
    vNormal = normalize(mat3(uProjView)*(skin*vec4(aNorm,0)).xyz);
}
