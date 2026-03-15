#include "src/phaseUnwrap/qualityMapGuideMethod.h"
#include <iostream>
#include <fstream>
#include "src/utils/utils.h"
#include <vector>

using namespace utils;
using namespace qualityMapGuideMethod;

int main() {
    // 图像尺寸
    const int rows = 100;
    const int cols = 100;

    // 1. 生成质量图（全为 1，即所有点质量相同）
    MatrixD qualityMap(rows, cols);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            qualityMap(i, j) = 0.9;
        }
    }

    // 2. 生成包裹相位：线性斜坡相位并包裹到 [-π, π]
    MatrixD phaseMap(rows, cols);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            double phase = 0.1 * i + 0.05 * j;
            phaseMap(i, j) = atan2(sin(phase), cos(phase));
        }
    }

    // // 3. 执行解缠（阈值设为 0，表示所有有效点均参与）
    Method method;
    double threshold = 0.0;
    auto unwrappedPtr = method.unwrap(qualityMap, phaseMap, threshold);

    if (!unwrappedPtr) {
        std::cerr << "解缠失败！可能没有点满足阈值。" << std::endl;
        return 1;
    }

    MatrixD& unwrapped = *unwrappedPtr;

    // 4. 将包裹相位和解缠相位写入 CSV 文件
    std::ofstream file_wrapped("wrapped_phase.csv");
    std::ofstream file_unwrapped("unwrapped_phase.csv");

    if (!file_wrapped.is_open() || !file_unwrapped.is_open()) {
        std::cerr << "无法创建 CSV 文件！" << std::endl;
        return 1;
    }

    // 写入包裹相位（每行代表图像的一行，元素用逗号分隔）
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            file_wrapped << phaseMap(i, j);
            if (j < cols - 1) file_wrapped << ",";
        }
        file_wrapped << "\n";
    }

    // 写入解缠相位
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            file_unwrapped << unwrapped(i, j);
            if (j < cols - 1) file_unwrapped << ",";
        }
        file_unwrapped << "\n";
    }

    file_wrapped.close();
    file_unwrapped.close();

    std::cout << "CSV 文件已生成：wrapped_phase.csv 和 unwrapped_phase.csv" << std::endl;
    return 0;
}