#include "utils.h"

namespace utils {

    // 默认的日志实现：输出到标准控制台 (Console)
    static void DefaultLogger(const char* msg) {
        // [Log] 前缀可根据需要保留或去除
        fprintf(stdout, "[Log] %s\n", msg);
        fflush(stdout);
    }

    // 全局函数指针，初始化为默认实现
    static LoggerCallback g_logger = DefaultLogger;

    void SetLogger(LoggerCallback cb) {
        if (cb) {
            g_logger = cb;
        } else {
            // 如果传入 nullptr，恢复默认
            g_logger = DefaultLogger;
        }
    }

    void Log(const char* fmt, ...) {
        char buffer[4096];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        if (g_logger) {
            g_logger(buffer);
        }
    }
}