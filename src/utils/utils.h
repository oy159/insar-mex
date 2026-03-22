#ifndef RADAR_ALGORITHM_UTILS_H
#define RADAR_ALGORITHM_UTILS_H

#include <cstdarg>
#include <vector>
#include <cstdint>
#include <memory>


#include <cstdio>
#include <cstdarg>

namespace utils {
    inline constexpr double PI = 3.14159265358979323846;
    inline constexpr double DEG2RAD = PI / 180.0;
    inline constexpr double RAD2DEG = 180.0 / PI;

    // 定义日志回调函数的类型指针
    using LoggerCallback = void(*)(const char* msg);

    /**
     * @brief 设置自定义的日志打印函数
     * MEX 环境下调用: utils::SetLogger([](const char* m){ mexPrintf("%s\n", m); });
     */
    void SetLogger(LoggerCallback cb);

    /**
     * @brief 通用日志输出函数
     * 内部会调用通过 SetLogger 注册的回调
     */
    void Log(const char* fmt, ...);




    template <typename T>
    inline constexpr T deg2rad(T d) { return d * static_cast<T>(DEG2RAD); }

    template <typename T>
    inline constexpr T rad2deg(T r) { return r * static_cast<T>(RAD2DEG); }

    template<typename T>
        class Matrix {
            std::vector<T> data_;
            size_t rows_, cols_;
            public:
                Matrix(size_t rows = 0, size_t cols = 0, T val = T()) : rows_(rows), cols_(cols), data_(rows * cols, val) {}
                size_t rows() const { return rows_; }
                size_t cols() const { return cols_; }

                T& operator()(size_t i, size_t j) { return data_[j * rows_ + i]; }
                const T& operator()(size_t i, size_t j) const { return data_[j * rows_ + i]; }

                std::vector<T>& data() { return data_; }
                const std::vector<T>& data() const { return data_; }
            // 提供 reshape 或 resize 方法等
    };

    using MatrixD = Matrix<double>;
    using MatrixB = Matrix<uint8_t>;

    struct PointVal {
        int x, y;        // 坐标
        double val;      // 质量值
    };

    struct ComparePointVal {
        bool operator()(const PointVal& a, const PointVal& b) const {
            return a.val < b.val;
        }
    };


};


#endif //RADAR_ALGORITHM_UTILS_H
