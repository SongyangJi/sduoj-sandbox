# 记录本项目中用到的系统调用

## 读写文件相关
#### write 
+ 头文件:
```c
#include <unistd.h>
```

+ 函数说明:
  write系统调用，是把缓存区buf中的前nbytes字节写入到与文件描述符有关的文件中。

+ 函数定义
```c
//参数分别为 文件描述符、缓冲区、
size_t write(int flides, const void *buf, size_t nbytes);
```

+ 返回值
  write系统调用返回的是实际写入到文件中的字节数。

### read  
+ 头文件:
```c
#include <unistd.h>
```

+ 函数说明:
  read系统调用，是从与文件描述符flides相关联的文件中读取前nbytes字节的内容，并且写入到数据区buf中。

+ 函数定义
```c
size_t read(int flides, void *buf, size_t nbytes);
```

+ 返回值
  read系统调用返回的是实际读入的字节数。
  

### open
+ 头文件:
```c
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
```

+ 函数说明:


+ 函数定义
```c
int open(const *path, int oflags); //1
int open(const *path, int oflags, mode_t mode); //2
```

参数说明：
其中，oflags是由必需文件访问模式和可选模式一起构成的(通过按位或“|”)：
必需部分：
- O_RDONLY———-以只读方式打开
- O_WRONLY———以只写方式打开
- O_RDWR————以读写方式打开
可选部分：
- O_CREAT————按照参数mode给出的访问模式创建文件
- O_EXCL————–与O_CREAT一起使用，确保创建出文件，避免两个程序同时创建同一个文件，如文件存在则open调用失败 
- O_APPEND———-把写入数据追加在文件的末尾
- O_TRUNC———–把文件长度设置为0，丢弃原有内容

在第一种调用方式上，加上了第三个参数mode，**主要是搭配O_CREAT使用**，同样地，这个参数规定了属主、同组和其他人对文件的文件操作权限。
+ 文件属主
  - S_IRUSR———-读权限 
  - S_IWUSR———写权限
  - S_IXUSR———-执行权限 
+ 文件所属组  
  - S_IRGRP———-读权限 
  - S_IWGRP———写权限 
  - S_IXGRP———-执行权限
+ 其他人  
  - S_IROTH———-读权限 
  - S_IWOTH———写权限
  - S_IXOTH———-执行权限 

另外，也可以用数字设定法：
0 : 无权限；
1 : 只执行；
2 : 只写；
4 : 只读。

这种权限设计实际上就是linux文件权限的设计。


+ 返回值
  open系统调用成功返回一个新的文件描述符，失败返回-1。


### close

+ 函数定义
```c
#include <unistd.h>
int close(int flides);
```

+ 函数说明:
  终止文件描述符flides与其对应的文件间的联系，文件描述符被释放，可重新使用。
  使用完文件描述符之后，要记得释放！



### fopen
+ 头文件:
```c
#include <stdio.h>
```
+ 函数说明:
  C 库函数,使用给定的模式 mode 打开 filename 所指向的文件。

+ 函数定义
```c
FILE *fopen(const char *filename, const char *mode)
```
| "r"  | 打开一个用于读取的文件。该文件必须存在。                     |
| ---- | ------------------------------------------------------------ |
| "w"  | 创建一个用于写入的空文件。如果文件名称与已存在的文件相同，则会删除已有文件的内容，文件被视为一个新的空文件。 |
| "a"  | 追加到一个文件。写操作向文件末尾追加数据。如果文件不存在，则创建文件。 |
| "r+" | 打开一个用于更新的文件，可读取也可写入。该文件必须存在。     |
| "w+" | 创建一个用于读写的空文件。                                   |
| "a+" | 打开一个用于读取和追加的文件。                               |

+ 返回值 
  该函数返回一个 FILE 指针。否则返回 NULL，且设置全局变量 errno 来标识错误。

### fclose
+ 头文件:
```c
#include <stdio.h>
```
+ 函数说明:
  C 库函数 int fclose(FILE *stream) 关闭流 stream, 并且刷新所有的缓冲区。

+ 函数定义
```c
// stream -- 这是指向 FILE 对象的指针，该 FILE 对象指定了要被关闭的流。
int fclose(FILE *stream)
```

+ 返回值
  如果流成功关闭，则该方法返回零。如果失败，则返回 EOF。

### fprintf
+ 头文件:
```c
#include <stdio.h>
```
+ 函数说明:
  C 库函数, 发送格式化输出到流 stream 中。

+ 函数定义
```c
int fprintf(FILE *stream, const char *format, ...)
```

+ 返回值
  如果成功，则返回写入的字符总数，否则返回一个负数。

### flock
+ 头文件 
```c
#include <sys/file.h>
```

+ 函数说明 flock()会依参数operation所指定的方式对参数fd所指的文件做各种锁定或解除锁定的动作。
  此函数只能锁定整个文件，无法锁定文件的某一区域。
  
+ 函数定义
```c
// fd 文件描述符、 锁定operation
int flock(int fd,int operation);
```

参数 operation 有下列四种情况:
  - LOCK_SH 建立共享锁定。多个进程可同时对同一个文件作共享锁定。
  - LOCK_EX 建立互斥锁定。一个文件同时只有一个互斥锁定。
  - LOCK_UN 解除文件锁定状态。
  - LOCK_NB 无法建立锁定时，此操作可不被阻断，马上返回进程。(通常与LOCK_SH或LOCK_EX做OR组合)

+ 返回值 
  返回0表示成功，若有错误则返回-1，错误代码存于errno。
  

#### snprintf
+ 头文件:
```c
#include <stdio.h>
```

+ 函数说明:
  C 库函数，将可变参数(...)按照 format 格式化成字符串，并将字符串复制到 str 中，size 为要写入的字符的最大数目，超过 size 会被截断。

+ 函数定义
```c
int snprintf (char * str, size_t size, const char * format, ... );
```

  - str -- 目标字符串。
  - size -- 拷贝字节数(Bytes)。
  - format -- 格式化成字符串。
  - ... -- 可变参数。

+ 返回值
  - 如果格式化后的字符串长度小于等于 size，则会把字符串全部复制到 str 中，并给其后添加一个字符串结束符 \0。
  返回的实际写入的长度。
  - 如果格式化后的字符串长度大于 size，超过 size 的部分会被截断，只将其中的 (size-1) 个字符复制到 str 中，并给其后添加一个字符串结束符 \0。
  返回值为欲写入的字符串长度。
    

## 重定向
### dup
+ 头文件:
```c
#include <unistd.h>
```

+ 函数说明:
  dup用来复制参数oldfd所指的文件描述符。
  返回的新文件描述符和参数oldfd指向同一个文件，这**两个描述符共享同一个数据结构，共享所有的锁定，读写指针和各项标志位**。


+ 函数定义
```c
int dup(int oldfd);
```

+ 返回值
  当复制成功是，返回最小的尚未被使用过的文件描述符;
  若有错误则返回-1。
  错误代码存入errno中。

### dup2
+ 头文件:
```c
#include <unistd.h>
```

+ 函数说明:
  dup2与dup区别是dup2可以用参数newfd指定新文件描述符的数值。
  若参数newfd已经被程序使用，则系统就会将newfd所指的文件关闭；
  若newfd等于oldfd，则返回newfd,而不关闭newfd所指的文件。
  dup2所复制的文件描述符与原来的文件描述符共享各种文件状态。共享所有的锁定，读写位置和各项权限或flags等.
  在shell的重定向功能中，(输入重定向”<”和输出重定向”>”)就是通过调用dup或dup2函数对标准输入和标准输出的操作来实现的。
  
+ 函数定义
```c
int dup2(int oldfd, int newfd);
```

+ 返回值
  若dup2调用成功则返回新的文件描述符，出错则返回-1。


+ 举例：
如何使用dup2实现标准输出到文件的重定向？
```c
#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>

int main() {
    int oldfd;
    int fd;
    int t;
    char *buf = "This is a test!!!!\n";
    if ((oldfd = open("/Users/jisongyang/CLionProjects/test_syscalls_sandbox/redirect/mine.txt", O_RDWR | O_CREAT,
                      0644)) == -1) {
        printf("open error\n");
        exit(-1);
    }
    fd = dup2(oldfd, STDOUT_FILENO);
    if (fd == -1) {
        printf("dup2 error\n");
        exit(-1);
    } else {
        printf("fd:%d  STDOUT_FILENO:%d\n", fd, STDOUT_FILENO);
    }
    t = (int) strlen(buf);
    if (write(fileno(stdout), buf, t) != t)//本应该写入到stdout的信息，但是标准输出已经重定向到目标文件中，故向标准输出写的数据将会写到目标文件中。
    {
        printf("write error!\n");
        exit(-1);
    }
    fflush(stdout); // printf 是带缓冲的函数，不加这一行代码，printf 的内容不会写到文件里
    close(fd);
    return 0;
}
```

最重要的一行代码：`dup2(oldfd, STDOUT_FILENO)`

## 时间相关
### time
+ 头文件:
```c
#include <time.h>
```
+ 函数说明:
  C 库函数: 返回自纪元 Epoch（1970-01-01 00:00:00 UTC）起经过的时间，以秒为单位。
  如果 seconds 不为空，则返回值也存储在变量 seconds 中。
+ 函数定义
```c
time_t time(time_t *seconds)
```
+ 返回值
  以 time_t 对象返回当前日历时间。
  
### localtime
+ 头文件:
```c
#include <time.h>
```

+ 函数说明:
  C 库函数  使用 timer 的值来填充 tm 结构。
  timer 的值被分解为 tm 结构，并用本地时区表示。

+ 函数定义
```c
struct tm *localtime(const time_t *timer)
```

+ 返回值
  该函数返回指向 tm 结构的指针，该结构带有被填充的时间信息。下面是 tm 结构的细节:
  
```c
struct tm {
int tm_sec;         /* 秒，范围从 0 到 59                */
int tm_min;         /* 分，范围从 0 到 59                */
int tm_hour;        /* 小时，范围从 0 到 23                */
int tm_mday;        /* 一月中的第几天，范围从 1 到 31                    */
int tm_mon;         /* 月份，范围从 0 到 11                */
int tm_year;        /* 自 1900 起的年数                */
int tm_wday;        /* 一周中的第几天，范围从 0 到 6                */
int tm_yday;        /* 一年中的第几天，范围从 0 到 365                    */
int tm_isdst;       /* 夏令时                        */    
};
```

### strftime
+ 头文件:
```c
#include <time.h>
```

+ 函数说明:
  C 库函数,根据 format 中定义的格式化规则，格式化结构 timeptr 表示的时间，并把它存储在 str 中。

+ 函数定义
```c
size_t strftime(char *str, size_t maxsize, const char *format, const struct tm *timeptr)
```
  - str -- 这是指向目标数组的指针，用来复制产生的 C 字符串。
  - maxsize -- 这是被复制到 str 的最大字符数。
  - format -- 这是 C 字符串，包含了普通字符和特殊格式说明符的任何组合。这些格式说明符由函数替换为表示 tm 中所指定时间的相对应值。
具体格式详见：
    [参考链接](https://www.runoob.com/cprogramming/c-function-strftime.html)

+ 返回值
  如果产生的 C 字符串小于 size 个字符（包括空结束字符），则会返回复制到 str 中的字符总数（不包括空结束字符），否则返回零。


## syscall

[Linux 下系统调用的三种方法](https://www.cnblogs.com/hazir/p/three_methods_of_syscall.html)

## getpw 函数组
getpw 都是用于获取用户的一组函数
相关函数：getpw, fgetpwent, getpwent, getpwuid

### getpwnam
+ 头文件:
```c
#include <pwd.h>
```

+ 函数说明:
  函数说明：getpwnam()用来逐一搜索参数 name 指定的账号名称, 找到时便将该用户的数据以 passwd 结构体返回。
  passwd 结构请参考 getpwent()。
  
+ 函数定义
```c
struct passwd * getpwnam(const char * name);
```

+ 返回值
  返回 passwd 结构数据, 如果返回NULL 则表示已无数据, 或有错误发生。

+ 示例代码:
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
    char *username = "jisongyang";
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
/*
输出结果：
pw->pw_name = jisongyang
pw->pw_passwd = ********
pw->pw_uid = 501
pw->pw_gid = 20
pw->pw_gecos = 吉松阳
pw->pw_dir = /Users/jisongyang
pw->pw_shell = /bin/zsh
 */
```

## 时间相关
### time
+ 头文件:
```c
#include <time.h>
```
+ 函数说明:
  C 库函数,返回自纪元 Epoch（1970-01-01 00:00:00 UTC）起经过的时间，以秒为单位。如果 seconds 不为空，则返回值也存储在变量 seconds 中。

+ 函数定义
```c
time_t time(time_t *seconds)
```

+ 返回值
  以 time_t 对象返回当前日历时间。


### gettimeofday
+ 头文件:
```c
#include <sys/time.h>
```
+ 函数说明:
  返回当前距离1970年的秒数和微妙数，后面的tz是时区，一般不用（传 NULL 即可）。
+ 函数定义
```c
int gettimeofday(struct timeval *tv, struct timezone *tz);
```

### clock_gettime
+ 头文件:
```c
#include <time.h>
```

+ 函数说明:
  根据时钟模式，获取多种时间。

+ 函数定义
```c
int clock_gettime(clockid_t clock_id, struct timespec * tp );
```

+ CLOCK_REALTIME       0
  Systemwide realtime clock. 系统实时时间,随系统实时时间改变而改变。
  即从UTC1970-1-1 0:0:0开始计时,中间时刻如果系统时间被用户该成其他,则对应的时间相应改变

+ CLOCK_MONOTONIC     1
  Represents monotonic time. Cannot be set. 从系统启动这一刻起开始计时,不受系统时间被用户改变的影响
  用的是相对时间，它的时间是通过jiffies值来计算的。该时钟不受系统时钟源的影响，只受jiffies值的影响。
  也就是说它获得的时间戳是单调的。

+ CLOCK_PROCESS_CPUTIME_ID    2
  High resolution per-process timer. 本进程到当前代码系统CPU花费的时间

+ CLOCK_THREAD_CPUTIME_ID      3
  Thread-specific timer. 本线程到当前代码系统CPU花费的时间

+ CLOCK_REALTIME_HR                4
  High resolution version of CLOCK_REALTIME. 0 
  CLOCK_REALTIME 的 高精度版本
  
+ CLOCK_MONOTONIC_HR            5
  High resolution version of CLOCK_MONOTONIC.
  CLOCK_MONOTONIC 的高精度版本


+ 返回值
  时间结构`struct timespec`
  

+ 示例代码
```c
#include<stdio.h>
#include <sys/time.h>

int main() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    printf("gettimeofday : %ld, %d\n", tv.tv_sec,tv.tv_usec);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    printf("CLOCK_REALTIME: %ld, %ld\n", ts.tv_sec, ts.tv_nsec);

    //打印出来的时间跟 cat /proc/uptime 第一个参数一样
    clock_gettime(CLOCK_MONOTONIC, &ts);
    printf("CLOCK_MONOTONIC: %ld, %ld\n", ts.tv_sec, ts.tv_nsec);

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    printf("CLOCK_PROCESS_CPUTIME_ID: %ld, %ld\n", ts.tv_sec, ts.tv_nsec);

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    printf("CLOCK_THREAD_CPUTIME_ID: %ld, %ld\n", ts.tv_sec, ts.tv_nsec);

    printf("\n%ld\n", time(NULL));

    return 0;
}
```

值得一提的是，本项目使用的计时工具不能使用 time 以及 gettimeofday，否则有小概率发生"时间回溯现象"，
具体可以参考 [Linux的timedatectl —— 关闭或开启时间同步](https://song-yang-ji.blog.csdn.net/article/details/115837363).
必须使用 `clock_gettime(CLOCK_MONOTONIC, *timespec);` 才可以先后两次拿到的时间戳是递增的。