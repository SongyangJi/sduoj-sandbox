#ifndef SANDBOX_UTIL_H_
#define SANDBOX_UTIL_H_

#include "public.h"

/**
 * get uid and pid of role `nobody`
 * 获取 nobody 身份的用户
 * @param uid
 * @param gid
 */
void GetNobody(int *uid, int *gid);


/**
 * release arg table and exit
 * @param exit_code 退出状态码
 */
void Halt(int exit_code);


/**
 * receive unexpected arg and hit help
 * 接收到不合法的参数，给出帮助
 */
void UnexpectedArg();

/**
 * print how to use sandbox
 * 打印 sandbox 的使用方法
 */
void PrintUsage();

/**
 * print current sandbox version
 * 打印当前 sandbox 的版本
 */
void PrintVersion();

#endif //SANDBOX_UTIL_H_