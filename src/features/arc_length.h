#pragma once
#include "features/bezier_path.h"
#include <vector>

class ArcLengthTable {
public:
    explicit ArcLengthTable(const BezierPath& path);

    float totalLength() const;

    // path parameter t (0 to segmentCount) that lies this far along the curve
    float parameterAtDistance(float distance) const;

private:
    // curve length from the start up to each sample
    std::vector<float> m_lengths;
    float m_step { 0.0f };
};
