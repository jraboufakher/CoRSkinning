#pragma once

#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <glm/glm.hpp>
#include "FBXLoader.h"
#include <cor/CoRCalculator.h>


class CoRProcessor {
public:
    CoRProcessor(float sigma,
        float omega,
        bool performSubdivision,
        unsigned int numThreads,
        float subdivEpsilon = 0.5f,
        bool useBFS = false,
        float bfsEpsilon = 1e-6f);

    // Compute CoRs asynchronously.
    void ComputeCoRsAsync(const FBXLoader::FBXMeshData& mesh, unsigned int numBones, std::function<void(std::vector<glm::vec3>&)> callback);

    // Load CoRs from a binary file.
    std::vector<glm::vec3> LoadCoRsFromBinaryFile(const std::string& filepath) const;
    
    // Save CoRs to a binary file.
    void saveCoRsToBinaryFile(const std::string& filepath, std::vector<glm::vec3>& cors) const;

    // Save CoRs to a text file.
    void saveCoRsToTextFile(const std::string& filepath, std::vector<glm::vec3>& cors) const;

private:
    float sigma_;
    float omega_;
    bool  performSubdivision_;
    unsigned int numThreads_;
    float subdivEpsilon_;
    bool  useBFS_;
    float bfsEpsilon_;

    // Internal calculators
    std::unique_ptr<CoR::CoRCalculator>    calc_;
    std::unique_ptr<CoR::BFSCoRCalculator> bfsCalc_;
};
