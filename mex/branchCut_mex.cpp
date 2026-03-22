#include "mex.h"
#include "../src/phaseUnwrap/branchCut.h"
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
 *   [unwrapped, residues, branchCuts, numRes] = branchCut_mex(wrappedPhase);
 */
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    // 1. 输入检查
    if (nrhs != 1) {
        mexErrMsgIdAndTxt("branchCut:invalidNumInputs", "One input argument required: wrappedPhase.");
    }
    if (!mxIsDouble(prhs[0])) {
        mexErrMsgIdAndTxt("branchCut:invalidInputType", "Input wrappedPhase must be a double matrix.");
    }

    // 2. 将数据导入 C++
    utils::SetLogger(MexLoggerBridge);
    const mxArray* inputMx = prhs[0];
    size_t rows = mxGetM(inputMx);
    size_t cols = mxGetN(inputMx);

    utils::MatrixD wrappedPhase = mxArrayToMatrixD(inputMx);

    // 3. 执行核心算法
    branchCut::Method method;
    std::unique_ptr<utils::MatrixD> result = method.unwrap(wrappedPhase);

    // 4. 构造输出

    // Output 1: Unwrapped Phase
    if (nlhs >= 1) {
        plhs[0] = mxCreateDoubleMatrix(rows, cols, mxREAL);
        double* outPtr = mxGetPr(plhs[0]);
        if (result) {
            for (size_t c = 0; c < cols; ++c) {
                for (size_t r = 0; r < rows; ++r) {
                    outPtr[c * rows + r] = (*result)(r, c);
                }
            }
        }
    }

    // Output 2: Residues (int -> double for visualization)
    if (nlhs >= 2) {
        auto residues = method.getResidues();
        plhs[1] = mxCreateDoubleMatrix(rows, cols, mxREAL);
        double* resPtr = mxGetPr(plhs[1]);
        if (residues) {
            for (size_t c = 0; c < cols; ++c) {
                for (size_t r = 0; r < rows; ++r) {
                    resPtr[c * rows + r] = static_cast<double>((*residues)(r, c));
                }
            }
        }
    }

    // Output 3: Branch Cuts (Logical)
    if (nlhs >= 3) {
        auto cuts = method.getBranchCuts();
        plhs[2] = mxCreateLogicalMatrix(rows, cols);
        mxLogical* cutPtr = mxGetLogicals(plhs[2]);
        if (cuts) {
            for (size_t c = 0; c < cols; ++c) {
                for (size_t r = 0; r < rows; ++r) {
                    cutPtr[c * rows + r] = (*cuts)(r, c) ? true : false;
                }
            }
        }
    }

    // Output 4: Number of Residues
    if (nlhs >= 4) {
        plhs[3] = mxCreateDoubleScalar(static_cast<double>(method.getNumResidues()));
    }
}