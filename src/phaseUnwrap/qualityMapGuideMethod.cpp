#include "qualityMapGuideMethod.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

double qualityMapGuideMethod::Method::itohUnwrapNeighbor(double unwrapped, double wrapped) {
    return unwrapped + wrap2pi(wrapped - unwrapped);
}

double qualityMapGuideMethod::Method::wrap2pi(double phase) {
    return atan2(sin(phase), cos(phase));
}

void qualityMapGuideMethod::Method::addAdjoinPoints(const PointVal &point,  const MatrixD& qualityMap) {
    static const std::vector<std::pair<int, int>> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (const auto& dir : directions) {
        int newX = point.x + dir.first;
        int newY = point.y + dir.second;
        if (newX >= 0 && newX < flag.rows() && newY >= 0 && newY < flag.cols() && !flag(newX, newY) && mask(newX, newY) && visited.find({newX, newY}) == visited.end()) {
            adjoinList.push({newX, newY, qualityMap(newX, newY)});
            visited.insert({newX, newY});
        }
    }
}

std::unique_ptr<MatrixD> qualityMapGuideMethod::Method::unwrap(const MatrixD &qualityMap, MatrixD &phaseMap, double qualityThreshold) {
    visited.clear();
    setMask(qualityMap, qualityThreshold);
    size_t cnt = 0;
    auto p = getBestPoint(qualityMap);
    if (p.val < qualityThreshold)
        return nullptr;
    flag = MatrixB(qualityMap.rows(), qualityMap.cols(), 0);
    MatrixD unwrappedPhase(
        qualityMap.rows(),
        qualityMap.cols(),
        std::numeric_limits<double>::quiet_NaN()
    );
    unwrappedPhase(p.x, p.y) = phaseMap(p.x, p.y);

    size_t mod = std::max<size_t>(1, totalPoints / 100);

    flag(p.x, p.y) = 1;
    visited.insert({p.x, p.y});
    cnt++;
    addAdjoinPoints(p, qualityMap);

    while (!adjoinList.empty()) {
        // unwrapped process
        auto unwrap = adjoinList.top();
        adjoinList.pop();
        static const std::vector<std::pair<int, int>> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
        for (const auto& dir : directions) {
            int adjX = unwrap.x + dir.first;
            int adjY = unwrap.y + dir.second;
            if (adjX >= 0 && adjX < flag.rows() && adjY >= 0 && adjY < flag.cols() && flag(adjX, adjY)) {
                unwrappedPhase(unwrap.x, unwrap.y) = itohUnwrapNeighbor(unwrappedPhase(adjX, adjY), phaseMap(unwrap.x, unwrap.y));
                flag(unwrap.x, unwrap.y) = 1;
                cnt++;
                if (progressCb && cnt % mod == 0) {
                    progressCb(cnt, totalPoints);
                }
                addAdjoinPoints(unwrap, qualityMap);
                break;
            }
        }
    }

    return std::make_unique<MatrixD>(std::move(unwrappedPhase));
}



void qualityMapGuideMethod::Method::setMask(
    const MatrixD& qualityMap,
    double qualityThreshold)
{
    size_t rows = qualityMap.rows();
    size_t cols = qualityMap.cols();
    mask = MatrixB(rows, cols, 1);
    totalPoints = 0;
    if (qualityThreshold == 0) {
        totalPoints = rows * cols;
        return;
    }
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            mask(i, j) = (qualityMap(i, j) >= qualityThreshold);
            totalPoints += mask(i, j);
        }
    }
}

PointVal qualityMapGuideMethod::Method::getBestPoint(const MatrixD &qualityMap) {
    const auto& data = qualityMap.data();
    auto it = std::max_element(data.begin(), data.end());
    size_t idx = it - data.begin();
    size_t i = idx / qualityMap.cols();
    size_t j = idx % qualityMap.cols();
    return PointVal{static_cast<int>(i), static_cast<int>(j), *it};
}

std::unique_ptr<MatrixB> qualityMapGuideMethod::Method::getMask() {
    return std::make_unique<MatrixB>(mask);
}

std::unique_ptr<MatrixB> qualityMapGuideMethod::Method::getFlag() {
    return std::make_unique<MatrixB>(flag);
}
