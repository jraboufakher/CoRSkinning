#pragma once

#include <vector>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <fbxsdk.h>
#include "FBXLoader.h"  // for fbxToGlm

class AnimController {
public:
    AnimController();
    ~AnimController() = default;

    // Initialize from the FBX scene and its root node.
    void Initialize(const FBXLoader& loader);

    // Play/Pause
    void togglePlayback();

    // Advance the animation by deltaSec (seconds) and update bone matrices/quaternions.
    void update(double deltaSec);

    // Evaluate the pose at the given time (in seconds).
    void evaluateAt(double timeSec);

    bool isPlaying()      const { return animPlaying_; }
    double getCurrentTime() const { return animStartSec_ + animTimeAcc_; }
    double getStartTime()   const { return animStartSec_; }
    double getEndTime()     const { return animEndSec_; }
    const std::vector<glm::mat4>& getBoneMatrices()   const { return boneMatrices_; }
    const std::vector<glm::quat>& getBoneQuaternions() const { return boneQuaternions_; }

private:

    FbxAnimEvaluator* evaluator_ = nullptr;
    std::vector<FbxNode*> boneNodes_;
    std::vector<glm::mat4> bindPoseInverse_;

    std::vector<glm::mat4> boneMatrices_;
    std::vector<glm::quat> boneQuaternions_;

    bool   animPlaying_ = true;
    double animTimeAcc_ = 0.0;
    double animStartSec_ = 0.0;
    double animEndSec_ = 0.0;
};
