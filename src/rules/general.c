#include <stdio.h>
#include <seccomp.h>
#include <linux/seccomp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "../public.h"

int general_seccomp_rules(struct config *_config) {
    // 系统调用黑名单 clone、fork、vfork、kill
    int syscalls_blacklist[] = {SCMP_SYS(clone),
                                SCMP_SYS(fork), SCMP_SYS(vfork),
                                SCMP_SYS(kill),
#ifdef __NR_execveat
            SCMP_SYS(execveat)
#endif
    };
    int syscalls_blacklist_length = sizeof(syscalls_blacklist) / sizeof(int);
    scmp_filter_ctx ctx = NULL;
    // init seccomp rules
    ctx = seccomp_init(SCMP_ACT_ALLOW); // 默认所有的系统调用都是允许的，然后添加黑名单
    if (!ctx) {
        return LOAD_SECCOMP_FAILED;
    }

    for (int i = 0; i < syscalls_blacklist_length; i++) {
        if (seccomp_rule_add(ctx, SCMP_ACT_KILL, syscalls_blacklist[i], 0) != 0) {
            return LOAD_SECCOMP_FAILED;
        }
    }
    // use SCMP_ACT_KILL for socket, python will be killed immediately
    if (seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EACCES), SCMP_SYS(socket), 0) != 0) {
        return LOAD_SECCOMP_FAILED;
    }
    /**
     * 以下为 version 1.0.0 中的代码，
     * 意为：不允许使用 execve 系统调用，但是由于需要使用 execve(_config->exe_path, _config->exe_args, _config->exe_envs)
     * 所以
     */
    // add extra rule for execve
    if (seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(execve), 1 ,
                         SCMP_A0(SCMP_CMP_NE, (scmp_datum_t)(_config->exe_path)) ) != 0) {
        return LOAD_SECCOMP_FAILED;
    }


    // do not allow "w" and "rw" using open
    if (seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(open), 1, SCMP_CMP(1, SCMP_CMP_MASKED_EQ, O_WRONLY, O_WRONLY)) !=
        0) {
        return LOAD_SECCOMP_FAILED;
    }
    if (seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(open), 1, SCMP_CMP(1, SCMP_CMP_MASKED_EQ, O_RDWR, O_RDWR)) != 0) {
        return LOAD_SECCOMP_FAILED;
    }

    // do not allow "w" and "rw" using openat
    if (seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(openat), 1,
                         SCMP_CMP(2, SCMP_CMP_MASKED_EQ, O_WRONLY, O_WRONLY)) != 0) {
        return LOAD_SECCOMP_FAILED;
    }
    if (seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(openat), 1, SCMP_CMP(2, SCMP_CMP_MASKED_EQ, O_RDWR, O_RDWR)) !=
        0) {
        return LOAD_SECCOMP_FAILED;
    }

    // load ctx to bring it into effect
    if (seccomp_load(ctx) != 0) {
        return LOAD_SECCOMP_FAILED;
    }
    // release ctx
    seccomp_release(ctx);
    return 0;
}
