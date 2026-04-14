#include "mex.h"
#include "../src/phaseUnwrap/mcf.h"
#include "../src/utils/utils.h"

#include <vector>
#include <memory>

/**
 * MEX 接驳日志函数
 */
void MexLoggerBridge(const char* msg) {
    mexPrintf("%s\n", msg);
    // mexEvalString("drawnow;"); // 根据需要开启
}

/**
 * 辅助：MxArray -> utils::MatrixD (列主序适配)
 */
utils::MatrixD mxArrayToMatrixD(const mxArray* mxArr) {
    size_t rows = mxGetM(mxArr);
    size_t cols = mxGetN(mxArr);
    double* data = mxGetPr(mxArr);

    utils::MatrixD mat(rows, cols);

    memcpy(mat.data().data(), data, sizeof(double) * rows * cols);
    return mat;
}

/**
 * MEX 入口函数
 *
 * Usage:
 *   [unwrapped, residues, reliability] = mcf_mex(wrappedPhase, debugMode);
 */
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    // 1. 输入检查
    if (nrhs < 1 || nrhs > 2) {
        mexErrMsgIdAndTxt("mcf:invalidNumInputs", "One or two input arguments required: wrappedPhase, [debugMode].");
    }
    if (!mxIsDouble(prhs[0])) {
        mexErrMsgIdAndTxt("mcf:invalidInputType", "Input wrappedPhase must be a double matrix.");
    }

    bool debugMode = true;
    if (nrhs == 2) {
        if (!mxIsLogical(prhs[1]) && !mxIsDouble(prhs[1])) {
            mexErrMsgIdAndTxt("mcf:invalidInputType", "Input debugMode must be a logical or scalar double.");
        }
        debugMode = (mxGetScalar(prhs[1]) != 0.0);
    }

    // 2. 将数据导入 C++
    utils::SetLogger(MexLoggerBridge);
    const mxArray* inputMx = prhs[0];
    size_t rows = mxGetM(inputMx);
    size_t cols = mxGetN(inputMx);

    utils::MatrixD wrappedPhase = mxArrayToMatrixD(inputMx);

    // 3. 执行核心算法
    mcf::Method method;
    method.setDebugMode(debugMode);
    std::unique_ptr<utils::MatrixD> result = method.unwrap(wrappedPhase);

    // 4. 构造输出

    // Output 1: Unwrapped Phase
    if (nlhs >= 1) {
        plhs[0] = mxCreateDoubleMatrix(rows, cols, mxREAL);
        double* outPtr = mxGetPr(plhs[0]);
        if (result) {
            memcpy(outPtr, result->data().data(), sizeof(double) * rows * cols);
        }
    }

    // Output 2: Residues (M x N)
    if (nlhs >= 2) {
        plhs[1] = mxCreateDoubleMatrix(rows, cols, mxREAL);
        double* resPtr = mxGetPr(plhs[1]);
        if (debugMode && method.debugData.residues) {
            memcpy(resPtr, method.debugData.residues->data().data(), sizeof(double) * rows * cols);
        }
    }

    // Output 3: Reliability Map (M x N)
    if (nlhs >= 3) {
        plhs[2] = mxCreateDoubleMatrix(rows, cols, mxREAL);
        double* relPtr = mxGetPr(plhs[2]);
        if (debugMode && method.debugData.reliability) {
            memcpy(relPtr, method.debugData.reliability->data().data(), sizeof(double) * rows * cols);
        }
    }
}
