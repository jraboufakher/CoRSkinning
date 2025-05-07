#include "render/AnimController.h"
#include <glm/gtx/quaternion.hpp>
#include <iostream>

AnimController::AnimController() {}

void AnimController::Initialize(const FBXLoader& loader) {
    bones_ = loader.GetBones();

    size_t numBones = bones_.size();
    boneMatrices_.resize(numBones, glm::mat4(1.0f));
    boneQuaternions_.resize(numBones, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

    FbxScene* scene = loader.GetScene();
    if (scene->GetSrcObjectCount<FbxAnimStack>() > 0) {
        FbxAnimStack* stack = scene->GetSrcObject<FbxAnimStack>(0);
        scene->SetCurrentAnimationStack(stack);
        FbxTakeInfo* takeInfo = scene->GetTakeInfo(stack->GetName());
        animStartSec_ = takeInfo->mLocalTimeSpan.GetStart().GetSecondDouble();
        animEndSec_ = takeInfo->mLocalTimeSpan.GetStop().GetSecondDouble();
        evaluator_ = scene->GetAnimationEvaluator();
    }
    else {
        evaluator_ = nullptr;
        std::cerr << "No animation stacks found in FBX scene.\n";
    }

    animPlaying_ = false;
    animTimeAcc_ = 0.0;
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
    animTimeAcc_ = timeSec;
    evaluateHierarchy(timeSec);
}

void AnimController::evaluateHierarchy(double timeSec)
{
    if (!evaluator_) return;

    FbxTime fbxTime;
    fbxTime.SetSecondDouble(timeSec);

    // Temporary storage for local transforms
    std::vector<glm::mat4> localTransforms(bones_.size());

    for (size_t i = 0; i < bones_.size(); ++i) {
        FbxAMatrix fbxLocal = evaluator_->GetNodeLocalTransform(bones_[i].node, fbxTime);
        localTransforms[i] = FBXLoader::fbxToGlm(fbxLocal);
    }

    // Explicitly calculate global transforms using parent-child hierarchy
    for (size_t i = 0; i < bones_.size(); ++i) {
        int parentIdx = bones_[i].parentIndex;
        glm::mat4 parentGlobal = (parentIdx == -1) ? glm::mat4(1.0f) : boneMatrices_[parentIdx];

        // global = parentGlobal * local
        boneMatrices_[i] = parentGlobal * localTransforms[i];

        // Compute final skinning transform by applying inverse bind-pose
        boneMatrices_[i] = boneMatrices_[i] * bones_[i].bindPoseInverse;

        // Extract quaternion for use elsewhere
        boneQuaternions_[i] = glm::quat_cast(glm::mat3(boneMatrices_[i]));
    }
}