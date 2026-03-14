#include "mex.h"
#include "../src/phaseUnwrap/qualityMapGuideMethod.h"
#include <vector>

using namespace qualityMapGuideMethod;

// 将 MATLAB 矩阵（列优先）转换为行优先的 MatrixD
MatrixD mxArrayToMatrixD(const mxArray* arr) {
    size_t rows = mxGetM(arr);
    size_t cols = mxGetN(arr);
    double* pr = mxGetPr(arr);
    MatrixD mat(rows, cols);
    for (size_t j = 0; j < cols; ++j) {
        for (size_t i = 0; i < rows; ++i) {
            mat(i, j) = pr[j * rows + i];
        }
    }
    return mat;
}

// 将行优先的 MatrixD 转换为 MATLAB 矩阵（列优先）
mxArray* matrixDToMxArray(const MatrixD& mat) {
    size_t rows = mat.rows();
    size_t cols = mat.cols();
    mxArray* arr = mxCreateDoubleMatrix(rows, cols, mxREAL);
    double* pr = mxGetPr(arr);
    const std::vector<double>& data = mat.data();
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            pr[j * rows + i] = data[i * cols + j];
        }
    }
    return arr;
}

// 将行优先的 MatrixB 转换为 MATLAB logical 矩阵
mxArray* matrixBToMxLogical(const MatrixB& mat) {
    size_t rows = mat.rows();
    size_t cols = mat.cols();
    mxArray* arr = mxCreateLogicalMatrix(rows, cols);
    mxLogical* pl = mxGetLogicals(arr);
    const std::vector<uint8_t>& data = mat.data();
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            pl[j * rows + i] = (data[i * cols + j] != 0);
        }
    }
    return arr;
}

void mexProgressCallback(size_t current, size_t total) {
    // 计算百分比
    double percent = (current * 100.0) / total;
    // 输出到 MATLAB 命令窗口
    mexPrintf("Unwrapped %zu / %zu points (%.1f%%)\n", current, total, percent);
    // 强制刷新，避免输出缓冲
    mexEvalString("drawnow;");
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    // 输入个数检查
    if (nrhs < 2 || nrhs > 3) {
        mexErrMsgIdAndTxt("qualityMapGuideMethod:invalidNumInputs",
                          "Two or three inputs required.");
    }
    if (nlhs > 3) {
        mexErrMsgIdAndTxt("qualityMapGuideMethod:tooManyOutputs",
                          "At most three outputs allowed.");
    }

    // 输入类型检查
    if (!mxIsDouble(prhs[0]) || mxIsComplex(prhs[0])) {
        mexErrMsgIdAndTxt("qualityMapGuideMethod:notDouble",
                          "qualityMap must be a real double matrix.");
    }
    if (!mxIsDouble(prhs[1]) || mxIsComplex(prhs[1])) {
        mexErrMsgIdAndTxt("qualityMapGuideMethod:notDouble",
                          "phaseMap must be a real double matrix.");
    }

    // 维度一致性检查
    size_t rows = mxGetM(prhs[0]);
    size_t cols = mxGetN(prhs[0]);
    if (mxGetM(prhs[1]) != rows || mxGetN(prhs[1]) != cols) {
        mexErrMsgIdAndTxt("qualityMapGuideMethod:dimMismatch",
                          "qualityMap and phaseMap must have the same dimensions.");
    }

    // 读取阈值（可选）
    double threshold = 0.0;
    if (nrhs == 3) {
        if (!mxIsDouble(prhs[2]) || mxGetNumberOfElements(prhs[2]) != 1) {
            mexErrMsgIdAndTxt("qualityMapGuideMethod:thresholdNotScalar",
                              "threshold must be a scalar double.");
        }
        threshold = mxGetScalar(prhs[2]);
    }

    // 将输入转换为行优先矩阵
    MatrixD qualityMap = mxArrayToMatrixD(prhs[0]);
    MatrixD phaseMap   = mxArrayToMatrixD(prhs[1]);

    // 执行解缠
    Method method;
    method.setProgressCallback(mexProgressCallback);
    std::unique_ptr<MatrixD> unwrapped = method.unwrap(qualityMap, phaseMap, threshold);
    if (!unwrapped) {
        mexErrMsgIdAndTxt("qualityMapGuideMethod:unwrapFailed",
                          "Unwrapping failed: no point above threshold?");
    }

    // 输出解缠相位
    plhs[0] = matrixDToMxArray(*unwrapped);

    // 可选输出：mask
    if (nlhs >= 2) {
        std::unique_ptr<MatrixB> maskPtr = method.getMask();
        plhs[1] = matrixBToMxLogical(*maskPtr);
    }

    // 可选输出：flag
    if (nlhs >= 3) {
        std::unique_ptr<MatrixB> flagPtr = method.getFlag();
        plhs[2] = matrixBToMxLogical(*flagPtr);
    }
}