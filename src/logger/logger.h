#ifndef SANDBOX_LOGGER_H_
#define SANDBOX_LOGGER_H_

#define LOG_LEVEL_FATAL 0
#define LOG_LEVEL_WARNING 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_DEBUG 3
// 缓冲区 8K
#define LOG_BUFFER_SIZE 8192

/**
 * 打开日志
 * @return
 */
FILE *LogOpen(const char *);

/**
 * 关闭日志
 */
void LogClose(FILE *);

// 写日志
void LogWrite(int level, const char *source_filename, const int line_number, const FILE *log_fp, const char *, ...); // 注意这个可变参数的使用

// 几种不同级别的日志记录
#define LOG_INFO(log_fp, x...) LogWrite(LOG_LEVEL_INFO, __FILE__, __LINE__, log_fp, ##x)
#define LOG_WARNING(log_fp, x...) LogWrite(LOG_LEVEL_WARNING, __FILE__, __LINE__, log_fp, ##x)
#define LOG_FATAL(log_fp, x...) LogWrite(LOG_LEVEL_FATAL, __FILE__, __LINE__, log_fp, ##x)

#ifdef SANDBOX_DEBUG
#define LOG_DEBUG(log_fp, x...) LogWrite(LOG_LEVEL_DEBUG, __FILE__, __LINE__, log_fp, ##x)
#else
#define LOG_DEBUG(log_fp, x...)
#endif

#endif //SANDBOX_LOGGER_H_