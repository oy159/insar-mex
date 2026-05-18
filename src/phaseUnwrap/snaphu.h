#pragma once

#include "utils/utils.h"

#ifdef INSAR_MEX_SNAPHU_AVAILABLE

extern "C" {
int snaphu_unwrap(const float *wrappedphase_in,
                  const float *mag_in,
                  long nrow, long ncol,
                  int initmethod, int costmode,
                  float *output);
}

namespace snaphu {

enum InitMethod {
    MST = 1,
    MCF = 2
};

enum CostMode {
    TOPO = 1,
    DEFO = 2,
    SMOOTH = 3
};

inline utils::MatrixD unwrap(const utils::MatrixD &wrappedPhase,
                             InitMethod initMethod = MST,
                             CostMode costMode = TOPO,
                             const utils::MatrixD *magnitude = nullptr) {
    int nrow = wrappedPhase.rows();
    int ncol = wrappedPhase.cols();

    /* convert input to float row-major */
    std::vector<float> wrappedFloat(nrow * ncol);
    for (int i = 0; i < nrow; i++) {
        for (int j = 0; j < ncol; j++) {
            wrappedFloat[i * ncol + j] = static_cast<float>(wrappedPhase(i, j));
        }
    }

    std::vector<float> magFloat;
    const float *magPtr = nullptr;
    if (magnitude != nullptr) {
        magFloat.resize(nrow * ncol);
        for (int i = 0; i < nrow; i++) {
            for (int j = 0; j < ncol; j++) {
                magFloat[i * ncol + j] = static_cast<float>((*magnitude)(i, j));
            }
        }
        magPtr = magFloat.data();
    }

    /* allocate output */
    std::vector<float> outputFloat(nrow * ncol);

    /* call C wrapper */
    int ret = snaphu_unwrap(wrappedFloat.data(), magPtr,
                            nrow, ncol,
                            static_cast<int>(initMethod),
                            static_cast<int>(costMode),
                            outputFloat.data());

    if (ret != 0) {
        utils::Log("SNAPHU unwrap failed with error code %d", ret);
        return utils::MatrixD(0, 0);
    }

    /* convert output back to double column-major */
    utils::MatrixD result(nrow, ncol);
    for (int i = 0; i < nrow; i++) {
        for (int j = 0; j < ncol; j++) {
            result(i, j) = static_cast<double>(outputFloat[i * ncol + j]);
        }
    }

    return result;
}

} /* namespace snaphu */

#endif /* INSAR_MEX_SNAPHU_AVAILABLE */
