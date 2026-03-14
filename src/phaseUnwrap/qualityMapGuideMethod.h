#ifndef RADAR_ALGORITHM_QUALITYMAPGUIDEMETHOD_H
#define RADAR_ALGORITHM_QUALITYMAPGUIDEMETHOD_H

#include "../utils/utils.h"
#include <queue>
#include <unordered_set>

using namespace utils;
namespace qualityMapGuideMethod {

    struct pair_hash {
        std::size_t operator()(const std::pair<int, int>& p) const {
            // 经典的组合哈希方法：将两个整数混合
            auto h1 = std::hash<int>{}(p.first);
            auto h2 = std::hash<int>{}(p.second);
            return h1 ^ (h2 << 1);   // 或更复杂的混合，如 h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2))
        }
    };

    class Method {
        std::priority_queue<PointVal, std::vector<PointVal>, ComparePointVal> adjoinList;
        std::unordered_set<std::pair<int, int>, pair_hash> visited;
        MatrixB flag; MatrixB mask;
        size_t totalPoints = 0;
        double itohUnwrapNeighbor(double unwrapped, double wrapped);
        static double wrap(double phase);
        void addAdjoinPoints(const PointVal& point, const MatrixD& qualityMap);
        void setMask(const MatrixD& qualityMap, double qualityThreshold);
        PointVal getBestPoint(const MatrixD& qualityMap);


        using ProgressCallback = void(*)(size_t current, size_t total);
        ProgressCallback progressCb = nullptr;

        public:
            std::unique_ptr<MatrixD> unwrap(const MatrixD& qualityMap, MatrixD& phaseMap, double qualityThreshold = 0);
            std::unique_ptr<MatrixB> getMask();
            std::unique_ptr<MatrixB> getFlag();

            void setProgressCallback(ProgressCallback cb) { progressCb = cb; }

    };


}


#endif //RADAR_ALGORITHM_QUALITYMAPGUIDEMETHOD_H