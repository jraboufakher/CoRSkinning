#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <fbxsdk.h>
#include "FBXLoader.h"

class AnimController {
public:
    AnimController();

    void Initialize(const FBXLoader& loader);
    void togglePlayback();
    void update(double deltaSec);
    void evaluateAt(double timeSec);

    const std::vector<glm::mat4>& getBoneMatrices() const { return boneMatrices_; }

    double getStartTime() const { return animStartSec_; }
    double getEndTime() const { return animEndSec_; }
    bool isPlaying() const { return animPlaying_; }

private:
    void evaluateHierarchy(double timeSec);

    bool animPlaying_ = false;
    double animTimeAcc_ = 0.0;
    double animStartSec_ = 0.0;
    double animEndSec_ = 0.0;

    FbxAnimEvaluator* evaluator_ = nullptr;

    std::vector<FBXLoader::Bone> bones_; // Full bone info (including hierarchy)
    std::vector<glm::mat4> boneMatrices_;
    std::vector<glm::quat> boneQuaternions_;
};