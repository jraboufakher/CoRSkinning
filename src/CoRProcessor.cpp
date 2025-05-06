#include "CoRProcessor.h"

CoRProcessor::CoRProcessor(float sigma,
    float omega,
    bool performSubdivision,
    unsigned int numThreads,
    float subdivEpsilon,
    bool useBFS,
    float bfsEpsilon)
    : sigma_(sigma)
    , omega_(omega)
    , performSubdivision_(performSubdivision)
    , numThreads_(numThreads)
    , subdivEpsilon_(subdivEpsilon)
    , useBFS_(useBFS)
    , bfsEpsilon_(bfsEpsilon)
{
    // Instantiate the appropriate calculator
    if (useBFS_) {
        bfsCalc_ = std::make_unique<CoR::BFSCoRCalculator>(
            sigma_, omega_, performSubdivision_, numThreads_, bfsEpsilon_);
    }
    else {
        calc_ = std::make_unique<CoR::CoRCalculator>(
            sigma_, omega_, performSubdivision_, numThreads_);
    }
}

void CoRProcessor::ComputeCoRsAsync(const FBXLoader::FBXMeshData& mesh, unsigned int numBones, std::function<void(std::vector<glm::vec3>&)> callback)
{
    // Choose calculator
    auto& calculator = useBFS_ ? static_cast<CoR::CoRCalculator&>(*bfsCalc_) : *calc_;

    // Weight conversion and mesh creation
    std::vector<CoR::WeightsPerBone> weightsPerBone = calculator.convertWeights(numBones, mesh.boneIndices, mesh.boneWeights);
    CoR::CoRMesh corMesh = calculator.createCoRMesh(mesh.vertices, mesh.faces, weightsPerBone, subdivEpsilon_);

    // Async compute with user callback
    calculator.calculateCoRsAsync(corMesh, [this, callback](std::vector<glm::vec3>& cors) {
        this->saveCoRsToBinaryFile("../cor_output/bfs_cs.cors", cors);
        this->saveCoRsToTextFile("../cor_output/bfs_cs.txt", cors);
        });
}

std::vector<glm::vec3> CoRProcessor::LoadCoRsFromBinaryFile(
    const std::string& filepath) const
{
    auto& calculator = useBFS_ ? static_cast<CoR::CoRCalculator&>(*bfsCalc_) : *calc_;
    return calculator.loadCoRsFromBinaryFile(filepath);
}

void CoRProcessor::saveCoRsToBinaryFile(const std::string& filepath, std::vector<glm::vec3>& cors) const
{
    auto& calculator = useBFS_ ? static_cast<CoR::CoRCalculator&>(*bfsCalc_) : *calc_;
    calculator.saveCoRsToBinaryFile(filepath, cors);
}

void CoRProcessor::saveCoRsToTextFile(const std::string& filepath, std::vector<glm::vec3>& cors) const
{
    auto& calculator = useBFS_ ? static_cast<CoR::CoRCalculator&>(*bfsCalc_) : *calc_;
    calculator.saveCoRsToTextFile(filepath, cors);
}