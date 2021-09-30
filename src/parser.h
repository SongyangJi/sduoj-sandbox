#ifndef SANDBOX_PARSER_H_
#define SANDBOX_PARSER_H_

#include "argtable/argtable3.h"
#include "public.h"

#define INT_PLACEHOLDER "<n>"
#define STR_PLACEHOLDER "<str>"

/*
  所有选项, 如下（version 1.0.0） :

  --help                    Display help and exit.
  --version                 Display version info and exit.
  --max_cpu_time=<n>        Max cpu running time (ms).
  --max_real_time=<n>       Max real running time (ms).
  --max_memory=<str>        Max memory (byte).
  --max_stack=<str>         Max stack size (byte, default 16384K).
  --max_process_number=<n>  Max Process Number
  --max_output_size=<n>     Max Output Size (byte)
  --exe_path=<str>          Executable file path.
  --input_path=<str>        Input file path.
  --output_path=<str>       Output file path.
  --log_path=<str>          Log file path.
  --exe_args=<str>          Arguments for exectuable file.
  --exe_envs=<str>          Environments for executable file.
  --seccomp_rules=<str>     Seccomp rules.
  --uid=<n>                 UID for executable file (default `nobody`).
  --gid=<n>                 GID for executable file (default `nobody`)
 */

#define NUM_ALLOWED_ARG 18

/**
 * 文本类型
 */
struct arg_lit *help, *version;

struct arg_int
    *max_cpu_time,            /* maximum cpu time(ms) */
    *max_real_time,           /* maximum real time, include blocked time(ms) */
    *max_process_number,      /* max process number */
    *max_output_size,         /* max output size (byte) */
    *uid, *gid;               /* run sandbox in such uid and gid */

/**
 * 字符串类型
 */
struct arg_str
    *max_memory,              /* maximum virtual memory(byte) */
    *max_stack,               /* maximum stack size(byte), default 16384K */
    *exe_path,                /* executable file that sandbox will run */
    *input_path,              /* executable file will read in */
    *output_path,             /* executable file will print out */
    *log_path,                /* sandbox will print log */
    *error_path,              /* sandbox will print error log */
    *exe_args,                /* args and envs for executable file */
    *exe_envs,                /* environments for executable file. */
    *seccomp_rules;           /* additional seccomp_rules */

/**
 * arg_table的最后一个必需的结构体
 */
struct arg_end *end;

void *arg_table[NUM_ALLOWED_ARG + 1];

/* parse from argv */
void Initialize(int argc, char **argv, struct config *_config);

/* Initialize config from args */
void InitConfig(struct config *_config);

#endif //SANDBOX_PARSER_H_
