#include <pwd.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "parser.h"


/**
 *
 * 这个文件多处使用 arg_table的函数，详见官方使用教程
 */


/**
 * 使用 getpwnam("nobody") 获取它的 uid、gid
 */
void GetNobody(int *uid, int *gid) {
    struct passwd *nobody;
    nobody = getpwnam("nobody");
    if (!nobody) {
        *uid = 65534;
        *gid = 65534;
    } else {
        *uid = nobody->pw_uid;
        *gid = nobody->pw_gid;
    }
}


/**
 * 清理 arg_table
 * sandbox 停机
 * @param exit_code 退出状态码
 */
void Halt(int exit_code) {
    arg_freetable(arg_table, sizeof(arg_table) / sizeof(arg_table[0]));
    exit(exit_code);
}

/**
 * 打印出错误，并给出提示
 */
void UnexpectedArg() {
    arg_print_errors(stdout, end, PROJECT_NAME);
    printf("Try '%s --help' for more information.\n", PROJECT_NAME);
}

/**
 * 提示用法、语法
 */
void PrintUsage() {
    printf("Usage: %s", PROJECT_NAME);
    arg_print_syntax(stdout, arg_table, "\n\n");
    arg_print_glossary(stdout, arg_table, "  %-25s %s\n");
}

/**
 * 打印版本号
 */
void PrintVersion() {
    // tricky bit operation
    printf("Version: %d.%d.%d\n", (VERSION >> 16) & 0xff, (VERSION >> 8) & 0xff, VERSION & 0xff);
}