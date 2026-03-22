#ifndef RADAR_ALGORITHM_CIRCULARMEDIANFILTER_H
#define RADAR_ALGORITHM_CIRCULARMEDIANFILTER_H

#include "../utils/utils.h"

// todo: 效率低下需要优化
namespace CircularMedianFilter {
    std::unique_ptr<utils::MatrixD> circularMedianFilter(const utils::MatrixD& input, int radius);
}


#endif //RADAR_ALGORITHM_CIRCULARMEDIANFILTER_H