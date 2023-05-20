/*
 * Copyright (c) 2023-2023 HXU-YanRuiBing <772166784@qq.com> All rights reserved.
 */
#ifndef __CLOG_H__
#define __CLOG_H__

#include <gpt_config.h>

/*
 * Log classification
 */
typedef enum clog_level {
    CLOG_TRACE,
    CLOG_DEBUG,
    CLOG_INFO,
    CLOG_WARN,
    CLOG_ERROR,
    CLOG_FATAL
} clog_level;

struct clog {
    enum clog_level level;          /* The current level of this logger. 
                                     * Messages below it will be dropped. */
    int             fd;             /* log file handle*/
    char            filename[256];  /* log file name*/
    pthread_mutex_t mutex;          /* A mutex variable is represented by 
                                     * the pthread_mutex_t data type.*/
    char            fmt[256];       /* %f: Source file name generating the log call.
                                     * %n: Source line number where the log call was made.
                                     * %m: The message text sent to the logger (after printf formatting).
                                     * %d: The current date, formatted using the logger's date format.
                                     * %t: The current time, formatted using the logger's time format.
                                     * %l: The log level (one of "DEBUG", "INFO", "WARN", or "ERROR").
                                     * %%: A literal percent sign.*/
    unsigned int    tfmt;           /* Time format */
};

gpt_clog_t *gpt_clog_creat(const char *filename, uint64_t maxsize);
void gpt_clog_close(gpt_clog_t *clog);
void gpt_clog_info(gpt_clog_t *clog, clog_level level,
                const char *sfile, int sline, const char *fmt, ...) __attribute__((format(printf, 5, 6)));

#define GCLOG_TRACE(clog, fmt, ...) gpt_clog_info(clog, CLOG_TRACE, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define GCLOG_DEBUG(clog, fmt, ...) gpt_clog_info(clog, CLOG_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define GCLOG_INFO(clog, fmt, ...) gpt_clog_info(clog, CLOG_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define GCLOG_WARN(clog, fmt, ...) gpt_clog_info(clog, CLOG_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define GCLOG_ERROR(clog, fmt, ...) gpt_clog_info(clog, CLOG_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define GCLOG_FATAL(clog, fmt, ...) gpt_clog_info(clog, CLOG_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif