# Sandbox的安全机制如何设计？
对于Linux而言，它的风格是一贯的。沙盒是进程而不是线程，这点非常明确。

## 用户权限相关

### Nobody 用户
在许多Unix系统与类Unix系统（如Linux）中，nobody是一个没有任何权限的用户。
该用户不拥有任何文件，也没有任何特殊权限。某些系统还会定义类似的用户组“nogroup”。
示例：
```c
/**
 * @Author: 吉松阳
 * @Date: 2021/9/23
 * @Description: 
 */
#include <stdio.h>
#include <pwd.h>

int main() {
    struct passwd *pw;
    char *username = "nobody";
    pw = getpwnam(username);
    if (!pw) {
        printf("%s is not exist\n", username);
        return -1;
    }
    printf("pw->pw_name = %s\n", pw->pw_name);
    printf("pw->pw_passwd = %s\n", pw->pw_passwd);
    printf("pw->pw_uid = %d\n", pw->pw_uid);
    printf("pw->pw_gid = %d\n", pw->pw_gid);
    printf("pw->pw_gecos = %s\n", pw->pw_gecos);
    printf("pw->pw_dir = %s\n", pw->pw_dir);
    printf("pw->pw_shell = %s\n", pw->pw_shell);

    return 0;
}
```
下面是 MacOS Big Sur 上的 nobody 用户相关信息。
其中`/var/empty`表明它不拥有任何文件，`/usr/bin/false`表明它不能登录使用shell
```
pw->pw_name = nobody
pw->pw_passwd = *
pw->pw_uid = -2
pw->pw_gid = -2
pw->pw_gecos = Unprivileged User
pw->pw_dir = /var/empty
pw->pw_shell = /usr/bin/false
```

在运行oj用户的代码的时候，是以nobody的身份运行的，意味着它的权限非常有限，不能去执行那些危险的代码。

### root 用户
root用户，即系统的管理员。
sandbox程序本身需要 root权限。
如何区分呢？
将上面的代码中的用户名替换成 root, 输出为：

```shell
pw->pw_name = root
pw->pw_passwd = *
pw->pw_uid = 0
pw->pw_gid = 0
pw->pw_gecos = System Administrator
pw->pw_dir = /var/root
pw->pw_shell = /bin/sh
```
发现uid、gid 均为 0。
于是在运行沙箱之前判定一下程序的执行者的uid、gid是不是 root 用户即可。


## 资源限制

### setuid/setgid
+ background infomation
  内核为每个进程维护的三个UID值。
  这三个UID分别是实际用户ID(real uid)、有效用户ID(effective uid)、保存的设置用户ID(saved set-user ID)。
  其中 real uid 指的是运行某程序的实际用户ID（登录shell的那个用户的uid）；
  effective uid 指的是指当前进程是以哪个用户ID来运行的；
  保存的设置用户ID就是有效用户ID的一个副本，与SUID权限有关。
  一般情况下 real uid 和 effective uid 相同，但是使用`setuid`、`chmod +s`之后，二者就不一定相同了。
  
+ `setuid`函数定义
```c
#include <unistd.h>
int setuid(uid_t uid);
```

+ 函数说明：
  +（1） 如果进程具有超级用户权限，那么 `setuid(uid_t uid)`会将三种 uid 全部设置成参数uid；
   (启动sandbox其实就是要求以root身份启动的)
  + (2) 如果 uid 等于 real uid 或者 saved set-user ID, 那么只把 effective uid 修改成 uid；
  + (3) 两种情况都不满组足，返回 -1 , errno被设置为 EPERM。

+ 返回值
  执行成功则返回0； 失败则返回-1, 错误代码存于errno.

+ 使用场景
  + 降低权限，比如在sandbox中通过让程序以 nobody 的身份来运行。
  + 提高权限，但是最好注意在使用完 root 权限后建议马上执行setuid(getuid())，来抛弃root 权限，避免不必要的风险。


### setrlimit/getrlimit
+ 头文件:
```c
#include <sys/resource.h>
```


+ 函数说明:
  获取或设定资源使用限制。
  每种资源都有相关的软硬限制:**软限制**是内核强加给相应资源的限制值，**硬限制**是软限制的最大值。
  非授权调用进程只可以将其软限制指定为0~硬限制范围中的某个值，同时能不可逆转地降低其硬限制。
  授权进程可以任意改变其软硬限制。
  RLIM_INFINITY的值表示不对资源限制。



+ 函数定义
```c
int getrlimit(int resource, struct rlimit *rlim);
int setrlimit(int resource, const struct rlimit *rlim);
```

resource：可能的选择有
- RLIMIT_AS // 进程的最大虚内存空间，字节为单位。
- RLIMIT_CORE // 内核转存文件的最大长度。
- RLIMIT_CPU // 最大允许的CPU使用时间，秒为单位。当进程达到软限制，内核将给其发送**SIGXCPU**信号，这一信号的默认行为是终止进程的执行。
  然而，可以捕捉信号，处理句柄可将控制返回给主程序。
  如果进程继续耗费CPU时间，核心会以每秒一次的频率给其发送SIGXCPU信号。
  如果达到硬限制，那时将给进程发送 SIGKILL信号终止其执行。
- RLIMIT_DATA // 进程数据段的最大值。
- RLIMIT_FSIZE // 进程可建立的文件的最大长度。如果进程试图超出这一限制时，核心会给其发送**SIGXFSZ**信号，默认情况下将终止进程的执行。
- RLIMIT_LOCKS // 进程可建立的锁和租赁的最大值。
- RLIMIT_MEMLOCK // 进程可锁定在内存中的最大数据量，字节为单位。
- RLIMIT_MSGQUEUE // 进程可为POSIX消息队列分配的最大字节数。
- RLIMIT_NICE // 进程可通过setpriority() 或 nice()调用设置的最大完美值。
- RLIMIT_NOFILE // 指定比进程可打开的最大文件描述词大一的值，超出此值，将会产生EMFILE错误。
- RLIMIT_NPROC // 用户可拥有的最大进程数。
- RLIMIT_RTPRIO // 进程可通过sched_setscheduler 和 sched_setparam设置的最大实时优先级。
- RLIMIT_SIGPENDING // 用户可拥有的最大挂起信号数。
- RLIMIT_STACK // 最大的进程栈，以字节为单位。


rlimit：描述资源软硬限制的结构体，原型如下
```c
struct rlimit {
	rlim_t  rlim_cur;               /* current (soft) limit 软限制 */
	rlim_t  rlim_max;               /* maximum value for rlim_cur 硬限制 */
};
```


+ 返回值
  **成功执行时，返回 0 。失败返回 -1** 。
  errno被设为以下的某个值：
    - EFAULT：rlim指针指向的空间不可访问
    - EINVAL：参数无效
    - EPERM：增加资源限制值时，权能不允许


+ 参考链接：
  [getrlimit(2) — Linux manual page](https://man7.org/linux/man-pages/man2/getrlimit.2.html)



# seccomp
## 什么是seccomp

seccomp（全称 **secure computing mode**）是linux kernel从2.6.23版本开始所支持的一种安全机制。
seccomp是一种**内核中的安全机制**,正常情况下,程序可以使用所有的syscall,这是不安全的。
比如劫持程序流后通过execve的syscall来`getshell`。
通过seccomp我们可以在程序中禁用掉某些syscall,这样就算劫持了程序流也只能调用部分的syscall了.

**通过seccomp，我们限制程序使用某些系统调用，这样可以减少系统的暴露面，同时是程序进入一种“安全”的状态。**
详细介绍可参考seccomp内核文档(见参考链接)。

## 如何使用seccomp
seccomp可以通过系统调用ptrctl(2)或者通过系统调用seccomp(2)开启，前提是内核配置中开启了CONFIG_SECCOMP和CONFIG_SECCOMP_FILTER。

seccomp支持两种模式：**SECCOMP_MODE_STRICT** 和 **SECCOMP_MODE_FILTER**。
+ 在SECCOMP_MODE_STRICT模式下，进程不能使用`read(2)`、`write(2)`、`_exit(2)`和`sigreturn(2)`以外的其他系统调用。
+ 在SECCOMP_MODE_FILTER模式下，可以利用BerkeleyPacket Filter配置哪些系统调用及它们的参数可以被进程使用。


## 如何查看是否使用了seccomp
通常有两种方法：
利用`prctl(2)`的PR_GET_SECCOMP的参数获取当前进程的seccomp状态。
  - 返回值0表示没有使用seccomp;
  - 返回值2表示使用了seccomp并处于SECCOMP_MODE_FILTER模式； 
  - 其他情况进程会被SIGKILL信号杀死。

从Linux3.8开始，可以利用/proc/$pid/status中的Seccomp字段查看。如果没有seccomp字段，说明内核不支持seccomp。

+ 举例:
查看mysql服务的seccomp的状态，发现并没有进入安全限制模式。
```shell
cat /proc/`pidof mysqld`/status
# 输出
Name:	mysqld
......
Seccomp:	0
Seccomp_filters:	0
```
在sandbox环境下执行 python3 脚本，
查看次进程的seccomp的状态，发现进程处于SECCOMP_MODE_FILTER模式 。
```shell
Name:	python3
......
Seccomp:	2
Seccomp_filters:	1
```

+ 代码示例
使用 `syscall` 调用 execve，如果没有安全限制的话，会正常进入 shell
```c
#include<unistd.h>
#include<sys/syscall.h>
#include<seccomp.h>

int main() {

    scmp_filter_ctx ctx; // scmp 过滤上下文
    ctx = seccomp_init(SCMP_ACT_ALLOW); // 初始化过滤状态为允许所有系统调用
    seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(execve), 0); // 添加需要限制的系统调用
    seccomp_load(ctx); // 装载上下文

    char *filename = "/bin/sh";
    char *argv[] = {"/bin/sh", NULL};
    char *envp[] = {NULL};

    syscall(SYS_execve, filename, argv, envp); // execve
    return 0;
}
```
编译
```shell
gcc -o ban ban.c -l seccomp
# 须先实现安装 
# sudo apt install libseccomp-dev libseccomp2 seccomp
```
运行程序
```shell
songyangji@SongyangJi-Ubuntu-DeskStop:~/桌面$ ./ban
错误的系统调用 (核心已转储) # Bad system call (core dumped)
```


## api rule

### SCMP_SYS
根据系统调用名获取系统调用号，虽然你可以直接使用 `__NR_syscall` 直接指定，但是为了跨平台最好使用它获取。
```c
int SCMP_SYS(syscall_name);
```

### scmp_filter_ctx
结构体定义
```c
typedef void * scmp_filter_ctx;
```
seccmp的过滤器上下文，保存、传递了我们传入的系统调用过滤条件。

### seccomp_init

+ 函数说明:
  seccomp_init的作用就是初始化 scmp_filter_ctx结构。
  需要注意的是，任何其他libseccomp中的函数调用，必须在seccomp_init之后。
  

+ 函数定义
```c
scmp_filter_ctx seccomp_init(uint32_t def_action); 
```

+ 返回值
  成功返回scmp_filter_ctx（过滤器上下文） ctx；
  失败返回NULL

+ 参数说明
  def_action用于指定默认行为，有效动作值如下：（当线程调用了`seccomp`过滤规则中没有相关配置规则的系统调用时触发）
  
  - SCMP_ACT_KILL
    线程将会被内核以SIGSYS信号终止；
  - SCMP_ACT_KILL_PROCESS
    整个进程被终止；
  - SCMP_ACT_TRAP
    线程将会抛出一个SIGSYS信号；
  - SCMP_ACT_ERRNO(uint16_t errno)
    线程调用与筛选规则匹配的系统调用时，它将收到一个errno的返回值；
  - SCMP_ACT_TRACE(uint16_t msg_num)
    略
  - SCMP_ACT_LOG
    不会对调用系统调用的线程产生任何影响，但系统调用会被记录到日志。
  - SCMP_ACT_ALLOW
    不会对调用系统调用的线程产生任何影响。

### seccomp_rule_addXXX

#### 函数说明:
  这个函数组都会向当前seccomp过滤器添加新的过滤规则。

> 调用应用程序提供的所有过滤器规则被组合成一个联合，并带有额外的逻辑来消除冗余的系统调用过滤器。
> 例如，如果添加了一条规则，该规则允许给定的系统调用具有一组特定的参数值，
> 然后又添加了一条规则，该规则允许相同的系统调用而不管参数值如何，
> 那么第一个更具体的规则将有效地从过滤器中删除第二个更通用的规则。

#### 函数定义
```c
int seccomp_rule_add(scmp_filter_ctx ctx, uint32_t action,
                            int syscall, unsigned int arg_cnt, ...);
int seccomp_rule_add_exact(scmp_filter_ctx ctx, uint32_t action,
                                  int syscall, unsigned int arg_cnt, ...);

int seccomp_rule_add_array(scmp_filter_ctx ctx,
                                  uint32_t action, int syscall,
                                  unsigned int arg_cnt,
                                  const struct scmp_arg_cmp *arg_array);
int seccomp_rule_add_exact_array(scmp_filter_ctx ctx,
                                        uint32_t action, int syscall,
                                        unsigned int arg_cnt,
                                        const struct scmp_arg_cmp *arg_array);
```


#### 参数说明
+ 1. action有效动作值如下：（当线程调用了`seccomp`过滤规则中有相关配置规则的系统调用时触发）
  - SCMP_ACT_KILL
    线程将会被内核终止；
  - SCMP_ACT_KILL_PROCESS
    整个进程被终止；
  - SCMP_ACT_TRAP
    线程将会抛出一个SIGSYS信号；
  - SCMP_ACT_ERRNO(uint16_t errno)
    线程调用与筛选规则匹配的系统调用时，它将收到一个errno的返回值；
  - SCMP_ACT_TRACE(uint16_t msg_num)
    略
  - SCMP_ACT_LOG
    会对调用系统调用的线程产生任何影响，但系统调用会被记录到日志。
  - SCMP_ACT_ALLOW
    不会对调用系统调用的线程产生任何影响（也就是允许调用这个system call）。
  - SCMP_ACT_NOTIFY
    略
+ 2. arg_cnt 指定规则配置的系统调用的参数的匹配情况的个数（因为后面是一个变长数组）
+ 3. 边长数组的元素是 `scmp_arg_cmp` 结构体，定义如下。
  
  

系统调用的参数比较规则相关定义：
```c
/**
 * Comparison operators
 */
enum scmp_compare {
	_SCMP_CMP_MIN = 0,
	SCMP_CMP_NE = 1,		/**< not equal */
	SCMP_CMP_LT = 2,		/**< less than */
	SCMP_CMP_LE = 3,		/**< less than or equal */
	SCMP_CMP_EQ = 4,		/**< equal */
	SCMP_CMP_GE = 5,		/**< greater than or equal */
	SCMP_CMP_GT = 6,		/**< greater than */
	SCMP_CMP_MASKED_EQ = 7,		/**< masked equality */
	_SCMP_CMP_MAX,
};

/**
 * Argument datum
 */
typedef uint64_t scmp_datum_t;

/**
 * Argument / Value comparison definition
 */
struct scmp_arg_cmp {
	unsigned int arg;	/**< argument number, starting at 0 */
	enum scmp_compare op;	/**< the comparison op, e.g. SCMP_CMP_* */
	scmp_datum_t datum_a;
	scmp_datum_t datum_b;
};
```

有效比较操作值（ op ）如下：

- SCMP_CMP_NE
  参数值不等于基准值时匹配，例如：
  SCMP_CMP( arg , SCMP_CMP_NE , datum )
  
- SCMP_CMP_LT
  参数值小于基准值时匹配，例如：
  SCMP_CMP( arg , SCMP_CMP_LT , datum )

- SCMP_CMP_LE
  参数值小于或等于基准值时匹配，例如：
  SCMP_CMP( arg , SCMP_CMP_LE , datum )

- SCMP_CMP_EQ
  参数值等于基准值时匹配，例如：
  SCMP_CMP( arg , SCMP_CMP_EQ , datum )

- SCMP_CMP_GE
  参数值大于或等于基准值时匹配，例如：
  SCMP_CMP( arg , SCMP_CMP_GE , datum )

- SCMP_CMP_GT
  参数值大于基准值时匹配，例如：
  SCMP_CMP( arg , SCMP_CMP_GT , datum )

- SCMP_CMP_MASKED_EQ
  当掩码参数值等于掩码基准值时匹配，例如：
  SCMP_CMP( arg , SCMP_CMP_MASKED_EQ , mask , datum )
  

注意，scmp_arg_cmp 此结构不能直接生成，需要调用它提供的宏生成，有如下宏：
```c
struct scmp_arg_cmp SCMP_CMP(unsigned int arg,
                                    enum scmp_compare op, ...);
       struct scmp_arg_cmp SCMP_A0(enum scmp_compare op, ...);
       struct scmp_arg_cmp SCMP_A1(enum scmp_compare op, ...);
       struct scmp_arg_cmp SCMP_A2(enum scmp_compare op, ...);
       struct scmp_arg_cmp SCMP_A3(enum scmp_compare op, ...);
       struct scmp_arg_cmp SCMP_A4(enum scmp_compare op, ...);
       struct scmp_arg_cmp SCMP_A5(enum scmp_compare op, ...);

       struct scmp_arg_cmp SCMP_CMP64(unsigned int arg,
                                    enum scmp_compare op, ...);
       struct scmp_arg_cmp SCMP_A0_64(enum scmp_compare op, ...);
       struct scmp_arg_cmp SCMP_A1_64(enum scmp_compare op, ...);
       struct scmp_arg_cmp SCMP_A2_64(enum scmp_compare op, ...);
       struct scmp_arg_cmp SCMP_A3_64(enum scmp_compare op, ...);
       struct scmp_arg_cmp SCMP_A4_64(enum scmp_compare op, ...);
       struct scmp_arg_cmp SCMP_A5_64(enum scmp_compare op, ...);

       struct scmp_arg_cmp SCMP_CMP32(unsigned int arg,
                                    enum scmp_compare op, ...);
       struct scmp_arg_cmp SCMP_A0_32(enum scmp_compare op, ...);
       struct scmp_arg_cmp SCMP_A1_32(enum scmp_compare op, ...);
       struct scmp_arg_cmp SCMP_A2_32(enum scmp_compare op, ...);
       struct scmp_arg_cmp SCMP_A3_32(enum scmp_compare op, ...);
       struct scmp_arg_cmp SCMP_A4_32(enum scmp_compare op, ...);
       struct scmp_arg_cmp SCMP_A5_32(enum scmp_compare op, ...);

```
解释一下上面的这么多宏的功能分类依据，A{0-5}中的0、1、2、3、4、5用于指定系统调用的那个参数，
32还是64自然是指定32位机器还是64位机器，
SCMP_CMP的第一个参数`unsigned int arg`的功能就是`A{$arg_num}`中的`$arg_num`，
所有宏的第一个参数`op`就是用于指定比较的规则，如上已经介绍过。


#### 返回值
函数成功时返回零；
失败时返回负的errno值。


#### 举例
1. 
```c
seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(open), 1, SCMP_CMP(1, SCMP_CMP_MASKED_EQ, O_WRONLY | O_RDWR, 0))
```
指定`open(const *path, int oflags)`系统调用的`oflags`参数如果既没有O_WRONLY，也没有O_RDWR（二进制对应位），就是允许的，
换言之这条规则禁用掉了 open的 w、rw。

### seccomp_load

+ 函数说明:
  将ctx提供的seccomp过滤器加载到内核中；
  如果函数成功，新的 seccomp 过滤器将在函数返回时处于活动状态


+ 函数定义
```c
int seccomp_load(scmp_filter_ctx ctx);
```

+ 返回值 
  成功时返回0，失败时返回以下错误码：
  -ECANCELED
    There was a system failure beyond the control of the
  library.
  -EFAULT
    Internal libseccomp failure.
  -EINVAL 
    Invalid input, either the context or architecture token is invalid. 
  -ENOMEM 
    The library was unable to allocate enough memory. 
  -ESRCH 
    Unable to load the filter due to thread issues.


### seccomp_release(3)
+ 函数说明:
  释放ctx 中的 seccomp 过滤器结构的内存，该过滤器首先由seccomp_init(3)或seccomp_reset(3)初始化，
  并释放与给定 seccomp 过滤器上下文关联的任何内存。
  加载到内核中的任何 seccomp过滤器不受影响。

+ 函数定义
```c
void seccomp_release(scmp_filter_ctx ctx);
```


### 参考链接
  [Secure Computing with filters](https://www.kernel.org/doc/Documentation/prctl/seccomp_filter.txt)
  [seccomp_init(3)](https://man7.org/linux/man-pages/man3/seccomp_init.3.html)
  [seccomp_rule_add(3)](https://man7.org/linux/man-pages/man3/seccomp_rule_add.3.html)
  [seccomp_load(3)](https://man7.org/linux/man-pages/man3/seccomp_load.3.html)
  [seccomp_release(3)](https://man7.org/linux/man-pages/man3/seccomp_release.3.html)


## prctl