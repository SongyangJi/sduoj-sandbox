#define _DEFAULT_SOURCE
#define _POSIX_SOURCE
#define _GNU_SOURCE

#include <grp.h>
#include <sched.h>
#include <errno.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/wait.h>
#include <sys/time.h>
#include <sys/mount.h>

#include "container.h"
#include "../logger/logger.h"
#include "../rules/seccomp_rules.h"

/**
 *
 * Initialize result to zero
 * 初始化结构体
 */
void InitResult(struct result *_result) {
    _result->cpu_time = 0;
    _result->real_time = 0;
    _result->memory = 0;
    _result->signal = 0;
    _result->exit_code = 0;
    _result->error = 0;
    _result->result = 0;
}

/**
 *
 * Check arguments
 * 检查参数变量
 *
 * @param log_fp 日志文件指针
 * @param _config 命令行输入参数
 * @param _result 结果
 */
void CheckArgs(FILE *log_fp, struct config *_config, struct result *_result) {
    // current user must be root
    if (getuid() != 0) ERROR_EXIT(ROOT_REQUIRED); // root 用户的 uid 为 0

    // check config （限制参数不能为非正数 ）
    if (_config->max_cpu_time < 1 || _config->max_real_time < 1 || _config->max_stack < 1 ||
        _config->max_memory < 1 || _config->max_process_number < 1 || _config->max_output_size < 1) ERROR_EXIT(
            INVALID_CONFIG);
}

/**
 * 创建子线程去监控用户进程，如果超时（max_real_time）, kill it
 * Create a new thread to kill the timeout process
 *
 */
void *KillTimeout(void *timeout_info) {
    // create a new thread to kill the timeout process
    pid_t pid = ((struct timeout_info *) timeout_info)->pid;
    int timeout = ((struct timeout_info *) timeout_info)->timeout;

    // pthread_detach(pthread_self()) set the thread's status to be unjoinable to release resources automatically; if success, return 0
    // sleep 使得当前进程暂停一段时间，如果返回值不等于 0，说明被信号中断（在本程序中是异常发生了）
    if (pthread_detach(pthread_self()) != 0 || sleep((unsigned int) ((timeout + 1000) / 1000)) != 0) {
        KillProcess(pid);
        return NULL;
    }

    // check in the end
    if (KillProcess(pid) != 0) // 最后兜底检查，杀死子进程
        return NULL;
    return NULL;
}

/**
 * 在子进程中运行用户的代码：先设置资源限制，配置seccmp规则，再使用 execve 执行用户代码
 * A function to run child process
 * @param log_fp 日志文件指针
 * @param _config
 */
void ChildProcess(FILE *log_fp, struct config *_config) {
    FILE *input_file = NULL, *output_file = NULL, *error_file = NULL;

    // set max stack space
    if (LIMITED(_config->max_stack)) {
        struct rlimit max_stack;
        max_stack.rlim_cur = max_stack.rlim_max = (rlim_t)(_config->max_stack);
        // 设置进程能使用的最大的栈空间，以字节为单位
        if (setrlimit(RLIMIT_STACK, &max_stack) != 0) CHILD_ERROR_EXIT(SETRLIMIT_FAILED);
    }

    // set memory limit
    if (LIMITED(_config->max_memory)) {
        struct rlimit max_memory;
        max_memory.rlim_cur = max_memory.rlim_max = (rlim_t)(_config->max_memory) * 2; // 预设为 2 倍，后面会判断
        // 设置进程能使用的最大虚拟内存空间，字节为单位
        if (setrlimit(RLIMIT_AS, &max_memory) != 0) CHILD_ERROR_EXIT(SETRLIMIT_FAILED);
    }

    // set cpu time limit (in seconds)
    if (LIMITED(_config->max_cpu_time)) {
        struct rlimit max_cpu_time;
        max_cpu_time.rlim_cur = max_cpu_time.rlim_max = (rlim_t)((_config->max_cpu_time + 1000) / 1000);  // ms 到 s 的转换
        // 设置进程能允许的最大CPU使用时间，秒为单位
        if (setrlimit(RLIMIT_CPU, &max_cpu_time) != 0) CHILD_ERROR_EXIT(SETRLIMIT_FAILED);
    }

    // set max process number limit
    if (LIMITED(_config->max_process_number)) {
        struct rlimit max_process_number;
        max_process_number.rlim_cur = max_process_number.rlim_max = (rlim_t) _config->max_process_number;
        // 设置进程能fork出最大进程数
        if (setrlimit(RLIMIT_NPROC, &max_process_number) != 0) CHILD_ERROR_EXIT(SETRLIMIT_FAILED);
    }

    // set max output size limit
    if (LIMITED(_config->max_output_size)) {
        struct rlimit max_output_size;
        max_output_size.rlim_cur = max_output_size.rlim_max = (rlim_t) _config->max_output_size;
        // 设置进程能建立的文件的最大长度
        if (setrlimit(RLIMIT_FSIZE, &max_output_size) != 0) CHILD_ERROR_EXIT(SETRLIMIT_FAILED);
    }

    if (_config->input_path != NULL) {
        input_file = fopen(_config->input_path, "r");
        if (input_file == NULL) CHILD_ERROR_EXIT(DUP2_FAILED);
        // redirect file -> stdin
        // On success, these system calls return the new descriptor.
        // On error, -1 is returned, and errno is set appropriately.
        if (dup2(fileno(input_file), fileno(stdin)) == -1) { // 标准输入读 -> 文件读
            CHILD_ERROR_EXIT(DUP2_FAILED);
        }
    }

    if (_config->output_path != NULL) {
        output_file = fopen(_config->output_path, "a"); // 追加写
        if (output_file == NULL) CHILD_ERROR_EXIT(DUP2_FAILED);
        // redirect stdout -> file
        if (dup2(fileno(output_file), fileno(stdout)) == -1) CHILD_ERROR_EXIT(DUP2_FAILED);
    }


    // TODO 重定向到 error.txt
    if (_config->error_path != NULL) {
        error_file = fopen(_config->error_path, "a"); // 追加写
        if (error_file == NULL) CHILD_ERROR_EXIT(DUP2_FAILED);
        // redirect stderr -> file
        if (dup2(fileno(error_file), fileno(stderr)) == -1) CHILD_ERROR_EXIT(DUP2_FAILED);
    }



    // 默认情况下设置成 nobody 身份
    // set gid
    gid_t group_list[] = {_config->gid};
    if (_config->gid != -1 && (setgid(_config->gid) == -1 ||
                               setgroups(sizeof(group_list) / sizeof(gid_t), group_list) == -1)) CHILD_ERROR_EXIT(
            SETUID_FAILED);
    // set uid
    if (_config->uid != -1 && setuid(_config->uid) == -1) CHILD_ERROR_EXIT(SETUID_FAILED);

    // load seccomp rules
    if (_config->seccomp_rules != NULL) {
        if (strcmp("c_cpp", _config->seccomp_rules) == 0 && c_cpp_seccomp_rules(_config) != SUCCESS) {
            CHILD_ERROR_EXIT(LOAD_SECCOMP_FAILED);
        } else if (strcmp("c_cpp_file_io", _config->seccomp_rules) == 0 &&
                   c_cpp_file_io_seccomp_rules(_config) != SUCCESS) {
            CHILD_ERROR_EXIT(LOAD_SECCOMP_FAILED);
        } else if (strcmp("general", _config->seccomp_rules) == 0 && general_seccomp_rules(_config) != SUCCESS) {
            CHILD_ERROR_EXIT(LOAD_SECCOMP_FAILED);
        } else if (strcmp("c_cpp", _config->seccomp_rules) != 0 &&
                   strcmp("c_cpp_file_io", _config->seccomp_rules) != 0 &&
                   strcmp("general", _config->seccomp_rules) != 0) {
            // rule does not exist
            CHILD_ERROR_EXIT(LOAD_SECCOMP_FAILED);
        }
    }

    // 执行用户的代码，如果可执行程序成功替代当前进程的地址空间，下面的 CHILD_ERROR_EXIT 不会执行
    execve(_config->exe_path, _config->exe_args, _config->exe_envs);  // 文件描述符父子进程共享，重定向生效
    // 如果 execve 执行失败，返回 -1， 执行下面的退出函数
    CHILD_ERROR_EXIT(EXECVE_FAILED);
}

/**
 * 父进程中开一个线程去监控子进程的运行状态以及资源使用情况
 *
 * Monitor process and require status and resource usage
 * @param log_fp 日志
 * @param child_pid 子进程id
 * @param _config 命令行参数
 * @param _result 结果
 * @param resource_usage 资源使用情况
 * @param status 子进程状态
 */
void RequireUsage(FILE *log_fp, pid_t child_pid, struct config *_config, struct result *_result,
                  struct rusage *resource_usage, int *status) {
    // create new thread to monitor process running time
    pthread_t tid = 0;
    if (LIMITED(_config->max_real_time)) {
        struct timeout_info timeout_args = {child_pid, _config->max_real_time};

        // 若线程创建成功，则返回0。若线程创建失败，则返回出错编号
        if (pthread_create(&tid, NULL, KillTimeout, (void *) (&timeout_args)) != 0) {
            // 如果出错， kill -9 child_pid
            KillProcess(child_pid);
            ERROR_EXIT(PTHREAD_FAILED);
        }
    }

    // wait for child process to terminate and require the status and resource usage of the child
    // if success, return the child process ID, else return -1
    if (wait4(child_pid, status, WSTOPPED, resource_usage) == -1) { // 注意, resource_usage参数会传出进程的资源使用情况
        KillProcess(child_pid);
        ERROR_EXIT(WAIT_FAILED);
    }

    // process exited, we may need to cancel KillTimeout thread
    if (LIMITED(_config->max_real_time)) // 兜底
        pthread_cancel(tid);
}

/* Generate the result */
void GenerateResult(FILE *log_fp, struct config *_config, struct result *_result, struct rusage *resource_usage, int *status,
               struct timespec *start, struct timespec *end) {
    // get end time
    clock_gettime(CLOCK_MONOTONIC, end);
    _result->real_time = (int) (end->tv_sec * 1000 + end->tv_nsec / 1000000
                                - start->tv_sec * 1000 - start->tv_nsec / 1000000);

    // if the child process terminated because it received a signal that was not handled, acquire the signal code
    if (WIFSIGNALED(*status) != 0) // 子进程异常退出
        _result->signal = WTERMSIG(*status); // 获取子进程异常退出的信号

    if (_result->signal == SIGUSR1) {   // 如果退出信号是 SIGUSR1，那说明是我们在 CHILD_ERROR_EXIT 中通过raise(sig)主动捕获异常发出的
        _result->result = SYSTEM_ERROR;
    } else {
        // 进入这个逻辑块，说明 sandbox 正常工作了，没有发生 SYSTEM_ERROR

        _result->exit_code = WEXITSTATUS(*status); // WEXITSTATUS的返回值即为子进程 exit(code) 中的 code
        // ... + resource_usage->ru_stime.tv_sec * 1000 + resource_usage->ru_stime.tv_usec / 1000
        _result->cpu_time = (int) (resource_usage->ru_utime.tv_sec * 1000 + resource_usage->ru_utime.tv_usec / 1000); // user time + system time maybe wrong
        _result->memory = resource_usage->ru_maxrss * 1024; /* bytes here, ru_maxrss is in kilobytes (in Linux) */

        if (_result->exit_code != 0)
            _result->result = RUNTIME_ERROR; // 运行时错误是一个 general 的 exit_code，下面进行特判

        if (_result->signal == SIGSEGV) { // 内存引用段错误（可能1.超内存 2.内存的非法访问）
            if (LIMITED(_config->max_memory) && _result->memory > _config->max_memory)
                _result->result = MEMORY_LIMIT_EXCEEDED; // 超内存
            else
                _result->result = RUNTIME_ERROR;
        } else {
            if (_result->signal != 0)
                _result->result = RUNTIME_ERROR;

            // 是否超内存
            if (LIMITED(_config->max_memory) && _result->memory > _config->max_memory)
                _result->result = MEMORY_LIMIT_EXCEEDED;

            // 是否超时间
            if (LIMITED(_config->max_real_time) && _result->real_time > _config->max_real_time)
                _result->result = REAL_TIME_LIMIT_EXCEEDED;

            // 是否超cpu使用时间
            if (LIMITED(_config->max_cpu_time) && _result->cpu_time > _config->max_cpu_time)
                _result->result = CPU_TIME_LIMIT_EXCEEDED;
        }
    }

    LogClose(log_fp); // 关闭日志文件
}

/* Examine and run the code */
void Examine(struct config *_config, struct result *_result) {
    int status;
    pid_t child_pid;
    struct timespec start, end;
//    struct timeval start, end;
    struct rusage resource_usage;

    // initialize log
    FILE *log_fp = LogOpen(_config->log_path);

    // Check arguments
    CheckArgs(log_fp, _config, _result);

    // Initialize result to zero
    InitResult(_result);

    // 获取初始时间
    // TODO  改为 clock_gettime()
    clock_gettime(CLOCK_MONOTONIC, &start);

    // examine the process
    child_pid = fork(); // fork 子进程
    if (child_pid < 0) {
        ERROR_EXIT(FORK_FAILED);
    } else if (child_pid == 0) {
        // 在子进程中执行用户的代码
        ChildProcess(log_fp, _config);
    } else {
        RequireUsage(log_fp, child_pid, _config, _result, &resource_usage, &status);
        GenerateResult(log_fp, _config, _result, &resource_usage, &status, &start, &end);
    }
}
