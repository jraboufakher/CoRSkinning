#include "render/AnimController.h"
#include <glm/gtx/quaternion.hpp>
#include <iostream>

AnimController::AnimController() {}

void AnimController::Initialize(const FBXLoader& loader) {
    // Collect skeleton bones
    boneNodes_ = loader.GetSkeletonNodes();
    bindPoseInverse_ = loader.GetBindPoseInverse();
    int numBones = static_cast<int>(boneNodes_.size());

    // Allocate pose arrays
    boneMatrices_.assign(numBones, glm::mat4(1.0f));
    boneQuaternions_.assign(numBones, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

    // Set up animation evaluator and time range
    FbxScene* scene = loader.GetScene();
    if (scene->GetSrcObjectCount<FbxAnimStack>() > 0) {
        FbxAnimStack* stack = scene->GetSrcObject<FbxAnimStack>(0);
        scene->SetCurrentAnimationStack(stack);
        FbxTakeInfo* takeInfo = scene->GetTakeInfo(stack->GetName());
        animStartSec_ = takeInfo->mLocalTimeSpan.GetStart().GetSecondDouble();
        animEndSec_ = takeInfo->mLocalTimeSpan.GetStop().GetSecondDouble();
    }
    evaluator_ = scene->GetAnimationEvaluator();
}

void AnimController::togglePlayback() {
    animPlaying_ = !animPlaying_;
}

void AnimController::update(double deltaSec) {
    if (animPlaying_) {
        animTimeAcc_ += deltaSec;
        double duration = animEndSec_ - animStartSec_;
        if (animTimeAcc_ > duration)
            animTimeAcc_ = fmod(animTimeAcc_, duration);
        evaluateAt(animStartSec_ + animTimeAcc_);
    }
}

void AnimController::evaluateAt(double timeSec) {
    if (!evaluator_) return;
    FbxTime fbxTime;
    fbxTime.SetSecondDouble(timeSec);

    int numBones = static_cast<int>(boneNodes_.size());
    for (int i = 0; i < numBones; ++i) {
        // evaluate global transform at time
        FbxAMatrix gxf = evaluator_->GetNodeGlobalTransform(boneNodes_[i], fbxTime);
        glm::mat4 globalMat = FBXLoader::fbxToGlm(gxf);

        // compute skinning matrix = global * inverse_bind
        boneMatrices_[i] = globalMat * bindPoseInverse_[i];

        // extract rotation quaternion
        boneQuaternions_[i] = glm::quat_cast(glm::mat3(boneMatrices_[i]));
    }
}