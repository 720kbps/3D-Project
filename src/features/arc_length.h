#pragma once
#include "core/mesh.h"
#include "features/bezier_path.h"
#include <framework/shader.h>
#include <vector>

class ArcLengthTable {
public:
    explicit ArcLengthTable(const BezierPath& path);

    float totalLength() const;

    // path parameter t (0 to segmentCount) that lies this far along the curve
    float parameterAtDistance(float distance) const;

    // unlit spheres at equal t steps (orange) and at equal distance steps (cyan)
    void drawMarkers(const BezierPath& path, int count, const Shader& shader, const glm::mat4& viewProjection, const std::vector<GPUMesh>& sphereMesh) const;

private:
    // curve length from the start up to each sample
    std::vector<float> m_lengths;
    float m_step { 0.0f };
};
