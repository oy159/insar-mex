#include "mex.h"
#include "../src/phaseUnwrap/snaphu.h"
#include "../src/utils/utils.h"

#include <cstring>
#include <string>

void MexLoggerBridge(const char* msg) {
    mexPrintf("%s\n", msg);
}

utils::MatrixD mxArrayToMatrixD(const mxArray* mxArr) {
    size_t rows = mxGetM(mxArr);
    size_t cols = mxGetN(mxArr);
    double* data = mxGetPr(mxArr);
    utils::MatrixD mat(rows, cols);
    memcpy(mat.data().data(), data, sizeof(double) * rows * cols);
    return mat;
}

/*
 * Usage:
 *   unwrapped = snaphu_mex(wrappedPhase)
 *   unwrapped = snaphu_mex(wrappedPhase, magnitude)
 *   unwrapped = snaphu_mex(wrappedPhase, magnitude, initMethod, costMode)
 *
 *   initMethod: 'mst' (default) or 'mcf'
 *   costMode:   'topo' (default), 'defo', or 'smooth'
 */
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
#ifdef INSAR_MEX_SNAPHU_AVAILABLE
    if (nrhs < 1 || nrhs > 4) {
        mexErrMsgIdAndTxt("snaphu:invalidNumInputs",
            "1 to 4 inputs: wrappedPhase, [magnitude], [initMethod], [costMode].");
    }
    if (!mxIsDouble(prhs[0]) || mxIsComplex(prhs[0])) {
        mexErrMsgIdAndTxt("snaphu:invalidInputType",
            "wrappedPhase must be a real double matrix.");
    }

    utils::SetLogger(MexLoggerBridge);

    size_t rows = mxGetM(prhs[0]);
    size_t cols = mxGetN(prhs[0]);
    utils::MatrixD wrappedPhase = mxArrayToMatrixD(prhs[0]);

    /* optional magnitude */
    const utils::MatrixD* magPtr = nullptr;
    utils::MatrixD magnitude;
    int nextArg = 1;
    if (nrhs >= 2 && mxIsDouble(prhs[1]) && mxGetM(prhs[1]) == rows && mxGetN(prhs[1]) == cols) {
        magnitude = mxArrayToMatrixD(prhs[1]);
        magPtr = &magnitude;
        nextArg = 2;
    }

    /* default: MST + TOPO */
    snaphu::InitMethod initMethod = snaphu::MST;
    snaphu::CostMode costMode = snaphu::TOPO;

    /* parse initMethod string */
    if (nrhs > nextArg && mxIsChar(prhs[nextArg])) {
        char buf[16];
        mxGetString(prhs[nextArg], buf, sizeof(buf));
        std::string s(buf);
        if (s == "mcf" || s == "MCF") {
            initMethod = snaphu::MCF;
        } else if (s == "mst" || s == "MST") {
            initMethod = snaphu::MST;
        } else {
            mexErrMsgIdAndTxt("snaphu:invalidInitMethod",
                "initMethod must be 'mst' or 'mcf'.");
        }
        nextArg++;
    }

    /* parse costMode string */
    if (nrhs > nextArg && mxIsChar(prhs[nextArg])) {
        char buf[16];
        mxGetString(prhs[nextArg], buf, sizeof(buf));
        std::string s(buf);
        if (s == "defo" || s == "DEFO") {
            costMode = snaphu::DEFO;
        } else if (s == "smooth" || s == "SMOOTH") {
            costMode = snaphu::SMOOTH;
        } else if (s == "topo" || s == "TOPO") {
            costMode = snaphu::TOPO;
        } else {
            mexErrMsgIdAndTxt("snaphu:invalidCostMode",
                "costMode must be 'topo', 'defo', or 'smooth'.");
        }
    }

    /* run SNAPHU unwrap */
    utils::MatrixD result = snaphu::unwrap(wrappedPhase, initMethod, costMode, magPtr);

    /* output */
    if (nlhs >= 1) {
        if (result.rows() == 0) {
            mexErrMsgIdAndTxt("snaphu:unwrapFailed", "SNAPHU unwrap failed.");
        }
        plhs[0] = mxCreateDoubleMatrix(rows, cols, mxREAL);
        memcpy(mxGetPr(plhs[0]), result.data().data(), sizeof(double) * rows * cols);
    }
#else
    mexErrMsgIdAndTxt("snaphu:notAvailable",
        "SNAPHU library was not linked. Rebuild with INSAR_MEX_BUILD_SNAPHU=ON.");
#endif
}