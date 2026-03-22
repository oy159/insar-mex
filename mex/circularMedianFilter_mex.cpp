#include "mex.h"
#include "../src/filter/circularMedianFilter.h"
#include <memory>
#include <chrono>

using namespace std::chrono;

// 将 MATLAB 矩阵（列优先）转换为列优先的 utils::MatrixD
utils::MatrixD mxArrayToMatrixD(const mxArray* arr) {
    size_t rows = mxGetM(arr);
    size_t cols = mxGetN(arr);
    double* pr = mxGetPr(arr);
    utils::MatrixD mat(rows, cols);
    std::memcpy(mat.data().data(), pr, rows * cols * sizeof(double));
    return mat;
}

// 将列优先的 utils::MatrixD 转换为 MATLAB 矩阵
mxArray* matrixDToMxArray(const utils::MatrixD& mat) {
    size_t rows = mat.rows();
    size_t cols = mat.cols();
    mxArray* arr = mxCreateDoubleMatrix(rows, cols, mxREAL);
    double* pr = mxGetPr(arr);
    const std::vector<double>& data = mat.data();
    std::memcpy(pr, data.data(), rows * cols * sizeof(double));
    return arr;
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    // 输入个数检查：相位矩阵、半径、可选输出标志
    if (nrhs < 2 || nrhs > 3) {
        mexErrMsgIdAndTxt("circularMedianFilter:invalidNumInputs",
                          "Two or three inputs required: phase matrix, radius, [verbose=true].");
    }
    if (nlhs > 1) {
        mexErrMsgIdAndTxt("circularMedianFilter:tooManyOutputs",
                          "Only one output allowed.");
    }

    // 输入类型检查
    if (!mxIsDouble(prhs[0]) || mxIsComplex(prhs[0])) {
        mexErrMsgIdAndTxt("circularMedianFilter:inputNotDouble",
                          "Phase input must be a real double matrix.");
    }
    if (!mxIsDouble(prhs[1]) || mxGetNumberOfElements(prhs[1]) != 1) {
        mexErrMsgIdAndTxt("circularMedianFilter:radiusNotScalar",
                          "Radius must be a scalar double.");
    }

    // 获取半径并转换为整数
    double radius_double = mxGetScalar(prhs[1]);
    int radius = static_cast<int>(radius_double);
    if (radius < 0) {
        mexErrMsgIdAndTxt("circularMedianFilter:negativeRadius",
                          "Radius must be non-negative.");
    }

    // 处理可选输出标志（默认 true）
    bool verbose = true;
    if (nrhs == 3) {
        // 检查第三个输入是否为 logical 或 double 标量
        if (!mxIsLogical(prhs[2]) && !(mxIsDouble(prhs[2]) && !mxIsComplex(prhs[2]))) {
            mexErrMsgIdAndTxt("circularMedianFilter:verboseNotLogical",
                              "Verbose flag must be a logical scalar or double 0/1.");
        }
        if (mxGetNumberOfElements(prhs[2]) != 1) {
            mexErrMsgIdAndTxt("circularMedianFilter:verboseNotScalar",
                              "Verbose flag must be scalar.");
        }
        // 读取值：如果是 logical 直接读取，如果是 double 则非零即为 true
        if (mxIsLogical(prhs[2])) {
            verbose = mxGetLogicals(prhs[2])[0];
        } else {
            verbose = (mxGetScalar(prhs[2]) != 0);
        }
    }

    // 将输入矩阵转换为 utils::MatrixD
    utils::MatrixD input = mxArrayToMatrixD(prhs[0]);

    // 计时开始
    high_resolution_clock::time_point start_time;
    if (verbose) {
        start_time = high_resolution_clock::now();
    }


    std::unique_ptr<utils::MatrixD> outputPtr = CircularMedianFilter::circularMedianFilter(input, radius);
    if (!outputPtr) {
        mexErrMsgIdAndTxt("circularMedianFilter:filterFailed",
                          "Filtering failed.");
    }

    // 计时结束并输出信息（如果需要）
    if (verbose) {
        auto end_time = high_resolution_clock::now();
        duration<double> elapsed = end_time - start_time;
        mexPrintf("Input size: [%zu x %zu], filtering time: %.3f seconds\n",
                  input.rows(), input.cols(), elapsed.count());
        mexEvalString("drawnow;");  // 刷新输出
    }

    // 将结果转换回 MATLAB 矩阵并返回
    plhs[0] = matrixDToMxArray(*outputPtr);
}
