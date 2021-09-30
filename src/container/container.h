#ifndef SANDBOX_EXAMINER_H_
#define SANDBOX_EXAMINER_H_

#include "../public.h"

/**
 * 测试参数是否有限制
 */
#define LIMITED(args) (args != RLIM_INFINITY)

#define CLOSE_FILE(fp)  \
    {                   \
        if (fp != NULL) \
            fclose(fp); \
    }

#define LOG_ERROR(error_code) LOG_FATAL(log_fp, #error_code)

/**
 * 强制 kill 进程
 */
#define KillProcess(pid) kill(pid, SIGKILL)

/**
 * 打日志、修改退出时的 error code
 */
#define ERROR_EXIT(error_code)       \
    {                                \
        LOG_ERROR(error_code);       \
        _result->error = error_code; \
        LogClose(log_fp);            \
        return;                      \
    }

// strerror(errno) : 内部数组中搜索错误号 errno，并返回一个指向错误消息字符串的指针
#define CHILD_ERROR_EXIT(error_code)                                                                 \
    {                                                                                                \
        LOG_FATAL(log_fp, "System errno: %s; Internal errno: " #error_code, strerror(errno));        \
        CLOSE_FILE(input_file);                                                                      \
        if (output_file == error_file)                                                               \
        {                                                                                            \
            CLOSE_FILE(output_file);                                                                 \
        }                                                                                            \
        else                                                                                         \
        {                                                                                            \
            CLOSE_FILE(output_file);                                                                 \
            CLOSE_FILE(error_file);                                                                  \
        }                                                                                            \
        raise(SIGUSR1);                                                                              \
        exit(EXIT_FAILURE);                                                                          \
    }

// 实际传入的是 {child_pid, _config->max_real_time}
struct timeout_info {
    pid_t pid;
    int timeout;
};

// 在sandbox运行程序的入口
void Examine(struct config *, struct result *);

#endif //SANDBOX_EXAMINER_H_