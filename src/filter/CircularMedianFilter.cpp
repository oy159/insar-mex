#include "CircularMedianFilter.h"

#include <algorithm>
#include <complex>

std::unique_ptr<utils::MatrixD> CircularMedianFilter::circularMedianFilter(const utils::MatrixD &input, int radius) {
    auto res = utils::MatrixD(input.rows(), input.cols(), 0.0);
    constexpr double eps = 1e-10;  // 用于检测矢量和模是否过小

    for (size_t i = 0; i < input.rows(); ++i) {
        for (size_t j = 0; j < input.cols(); ++j) {
            std::vector<double> window_phases;
            std::complex<double> sum_cplx(0.0, 0.0);

            // 收集窗口内所有有效像素的相位
            for (int di = -radius; di <= radius; ++di) {
                for (int dj = -radius; dj <= radius; ++dj) {
                    int ni = static_cast<int>(i) + di;
                    int nj = static_cast<int>(j) + dj;
                    if (ni >= 0 && ni < static_cast<int>(input.rows()) && nj >= 0 && nj < static_cast<int>(input.cols())) {
                        double phi = input(ni, nj);
                        window_phases.push_back(phi);
                        sum_cplx += std::polar(1.0, phi);   // 累加单位复数
                    }
                }
            }

            // 若矢量和模过小（相位均匀分布），则保留原值（可改用其他策略）
            if (std::abs(sum_cplx) < eps) {
                res(i, j) = input(i, j);
                continue;
            }

            // 计算每个相位相对于平均方向的偏差
            std::vector<double> deviations;
            deviations.reserve(window_phases.size());
            for (double phi : window_phases) {
                std::complex<double> ratio = std::polar(1.0, phi) / sum_cplx;  // 复数除法
                deviations.push_back(std::arg(ratio));                         // 偏差角度
            }

            // 取偏差的中位数
            size_t mid = deviations.size() / 2;
            std::nth_element(deviations.begin(), deviations.begin() + mid, deviations.end());
            double median_dev = deviations[mid];

            // 组合平均方向与中位数偏差
            double avg_angle = std::arg(sum_cplx);
            double out = avg_angle + median_dev;

            // 包裹到 [-π, π)
            out = std::atan2(std::sin(out), std::cos(out));

            res(i, j) = out;
        }
    }

    return std::make_unique<utils::MatrixD>(std::move(res));
}
