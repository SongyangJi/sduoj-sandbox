#ifndef SANDBOX_PUBLIC_H_
#define SANDBOX_PUBLIC_H_

#include <sys/types.h>
#include <sys/resource.h>

#define PROJECT_NAME "sandbox"
#define VERSION 0x010001

/**
 * MAX_ARG `--exe_args`选项最大个数
 * MAX_ENV `--exe_envs`选项个数
 */
#define MAX_ARG 256
#define MAX_ENV 256

enum {
    SUCCESS,                /* everything is ok */
    INVALID_CONFIG,         /* invalid config */
    FORK_FAILED,            /* run fork() failed */
    PTHREAD_FAILED,         /* run child thread failed */
    WAIT_FAILED,            /* run wait4() failed */
    DUP2_FAILED,            /* run dup2() failed */
    SETRLIMIT_FAILED,       /* run setrlimit() failed */
    SETUID_FAILED,          /* run setuid() failed */
    LOAD_SECCOMP_FAILED,    /* load seccomp rules failed */
    EXECVE_FAILED,          /* run execve() failed */
    SPJ_ERROR,              /* run Special Judge failed */
    ROOT_REQUIRED,          /* sandbox needs root privilege */
    NOBODY_REQUIRED         /* user program needs run in NOBODY */
};

enum {
                                    /* 0 表示一切正常 */
    CPU_TIME_LIMIT_EXCEEDED = 1,    /* 分配的CPU时间超时 */
    REAL_TIME_LIMIT_EXCEEDED = 2,   /* 真实的运行时间超时 */
    MEMORY_LIMIT_EXCEEDED = 3,      /* 内存超出限制 */
    RUNTIME_ERROR = 4,              /* 运行时错误 */
    SYSTEM_ERROR = 5,               /* 系统错误 */
};

/*
 * rlim_t 64位整型数，来自 <sys/resource.h>
 */


/**
 *
 * 从命令行解析提取出的参数，保存在 config 结构体里
 *
 * --max_cpu_time=<n>        Max cpu running time (ms).
 * --max_real_time=<n>       Max real running time (ms).
 * --max_memory=<n>        Max memory (byte).
 * --max_stack=<n>         Max stack size (byte, default 16384K).
 * --max_process_number=<n>  Max Process Number
 * --max_output_size=<n>     Max Output Size (byte)
 *
 * --exe_path=<str>          Executable file path.
 * --input_path=<str>        Input file path.
 * --output_path=<str>       Output file path.
 * --log_path=<str>          Log file path.
 * --exe_args=<str>          Arguments for exectuable file.
 * --exe_envs=<str>          Environments for executable file.
 * --seccomp_rules=<str>     Seccomp rules.
 * --uid=<n>                 UID for executable file (default `nobody`).
 * --gid=<n>                 GID for executable file (default `nobody`)
 */
struct config {
    // as above
    rlim_t max_cpu_time;
    rlim_t max_real_time;
    rlim_t max_memory;
    rlim_t max_stack;
    rlim_t max_process_number;
    rlim_t max_output_size;

    char *exe_path;
    char *input_path;
    char *output_path;
    char *log_path;
    char *error_path;

    // 最多允许有 256 个运行参数
    char *exe_args[MAX_ARG + 1];
    char *exe_envs[MAX_ENV + 1];

    char *seccomp_rules;

    uid_t uid; /* 用户 id */
    gid_t gid; /* 用户组 id */
};

/**
 *
 * sandbox 的运行结果
 *
 * @param cpu_time  cpu分配的时间(单位ms)
 * @param real_time 真实运行时间(单位ms)
 * @param memory    占用内存(单位byte)
 * @param signal    信号量
 * @param exit_code 退出状态码
 * @param error     错误类型
 * @param result    结果类型(6个枚举值)
 */
struct result {
    int cpu_time;
    int real_time;
    int memory;
    int signal;
    int exit_code;
    int error;
    int result;
};

#endif //SANDBOX_PUBLIC_H_