/*
 * Copyright (c) 2023-2023 HXU-YanRuiBing <772166784@qq.com> All rights reserved.
 */
#include <gpt_config.h>

#define LOG_BUFF            4096
#define CLOG_DEFAULT_FORMAT "%d %t %f(%n): %l: %m\n"

static const char *const CLOG_LEVEL_NAMES[] = {
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

enum {
    MAXFILELEN  = 255,
    MAXFILESIZE = 1048576L, /* 1 MB */
};

static void
_gpt_clog_err(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

static inline const char *
_gpt_clog_basename(const char *path) {
    const char *slash = strrchr(path, '/');
    if (slash)
        path = slash + 1;
    return path;
}

static char *
_gpt_clog_time(const gpt_clog_t *clog, char *timestamp, size_t size)
{
    time_t t;
    time(&t);

    return get_time_fmt(timestamp, size, t, clog->tfmt);
}

static void
_gpt_clog_filename(char *dst, uint32_t size, const char *src)
{
    char    *path = NULL, *fname = NULL;
    char     name[32] = {0};
    time_t   tm;

    if ((path = path_normalize(src)) == NULL) {
        _gpt_clog_err("(clog): path normalize faild.\n");
        goto out;
    }

    // stitching path
    strncpy(dst, path, size);
    if (dst[strlen(dst) - 1] != '/')
        strcat(dst, "/");

    time(&tm);
    fname = get_time_fmt(name, sizeof(name), tm, gf_timefmt_day);
    strcat(dst, fname);

out:
    // copy memory, so free
    if (path) {
        free(path);
        path = NULL;
    }
    return;
}

static size_t
_gpt_clog_append_str(char **dst, char *orig_buf, const char *src, size_t cur_size)
{
    size_t new_size = cur_size;

    while (strlen(*dst) + strlen(src) >= new_size)
        new_size *= 2;

    if (new_size != cur_size) {
        if (*dst == orig_buf) {
            *dst = (char *)malloc(new_size);
            strncpy(*dst, orig_buf, new_size);
        } else {
            *dst = (char *)realloc(*dst, new_size);
        }
    }

    strcat(*dst, src);
    return new_size;
}

static size_t
_gpt_clog_append_time(const gpt_clog_t *clog,
                      char **dst, 
                      char *orig_buf, 
                      const char *fmt,
                      size_t cur_size)
{
    char    buf[256];
    size_t  ret = 0;
    char    *pbuf = NULL;

    pbuf = _gpt_clog_time(clog, buf, sizeof(buf));

    if (pbuf != NULL)
        return _gpt_clog_append_str(dst, orig_buf, pbuf, cur_size);
    return cur_size;
}

static size_t
_gpt_clog_append_int(char **dst, char *orig_buf, long int d, size_t cur_size)
{
    char buf[40]; /* Enough for 128-bit decimal */

    if (snprintf(buf, sizeof(buf), "%ld", d) >= 40)
        return cur_size;
    
    return _gpt_clog_append_str(dst, orig_buf, buf, cur_size);
}

/**
 * Set the format string for log messages.  Here are the substitutions you may
 * use:
 *
 *     %f: Source file name generating the log call.
 *     %n: Source line number where the log call was made.
 *     %m: The message text sent to the logger (after printf formatting).
 *     %d: The current date, formatted using the logger's date format.
 *     %t: The current time, formatted using the logger's time format.
 *     %l: The log level (one of "DEBUG", "INFO", "WARN", or "ERROR").
 *     %%: A literal percent sign.
 *
 * The default format string is CLOG_DEFAULT_FORMAT.
 * */
static char *
_gpt_clog_format(const gpt_clog_t *clog, 
                char buf[],
                size_t buf_size,
                const char *sfile,
                int sline, 
                const char *level,
                const char *message)
{
    size_t  i, fmtlen;
    size_t  cur_size = buf_size;
    char   *result = buf;
    enum { NORMAL, SUBST } state = NORMAL;

    fmtlen = strlen(clog->fmt);
    sfile = _gpt_clog_basename(sfile);
    result[0] = 0;
    for (i = 0; i < fmtlen; ++i) {
        if (state == NORMAL) {
            if (clog->fmt[i] == '%') {
                state = SUBST;
            } else {
                char str[2] = { 0 };
                str[0] = clog->fmt[i];
                cur_size = _gpt_clog_append_str(&result, buf, str, cur_size);
            }
        } else {
            switch (clog->fmt[i]) {
                case '%':
                    cur_size = _gpt_clog_append_str(&result, buf, "%", cur_size);
                    break;
                case 't':
                    cur_size = _gpt_clog_append_int(&result, buf, (long)pthread_self(), cur_size);
                    break;
                case 'd':
                    cur_size = _gpt_clog_append_time(clog, &result, buf, clog->fmt, cur_size);
                    break;
                case 'l':
                    cur_size = _gpt_clog_append_str(&result, buf, level, cur_size);
                    break;
                case 'n':
                    cur_size = _gpt_clog_append_int(&result, buf, sline, cur_size);
                    break;
                case 'f':
                    cur_size = _gpt_clog_append_str(&result, buf, sfile, cur_size);
                    break;
                case 'm':
                    cur_size = _gpt_clog_append_str(&result, buf, message, cur_size);
                    break;
            }
            state = NORMAL;
        }
    }
    return result;
}

static void 
_gpt_clog_write(const gpt_clog_t *clog,
                clog_level level,
                const char *sfile,
                int sline,
                const char *fmt,
                va_list ap)
{
    int             result = 0;
    char            buf[LOG_BUFF];
    char           *dynbuf = buf;
    char           *message;
    size_t          size = LOG_BUFF;
    va_list         ap_copy;
    pthread_t       pid;
    
    //pid = pthread_self();

    va_copy(ap_copy, ap);
    result = vsnprintf(dynbuf, size, fmt, ap);
    if ((size_t)result >= size) {
        size = result + 1;
        dynbuf = (char *)calloc(1, size);
        result = vsnprintf(dynbuf, size, fmt, ap_copy);
        if ((size_t)result >= size) {
            /* Formatting failed -- too large */
            _gpt_clog_err("(clog): Formatting failed (1).\n");
            va_end(ap_copy);
            free(dynbuf);
            return;
        }
    }
    va_end(ap_copy);

    {
        char msg[LOG_BUFF];
        message = _gpt_clog_format(clog, msg, LOG_BUFF, 
                                    sfile, sline, CLOG_LEVEL_NAMES[level], dynbuf);
        if (!message) {
            _gpt_clog_err("(clog): Formatting failed (2).\n");
            if (dynbuf != buf)
                free(dynbuf);
            return;
        }
        /*write log message to file*/
#ifdef __GPTCLOG__
        result = write(clog->fd, message, strlen(message));
        if (result == -1)
            _gpt_clog_err("(clog): Unable to write to log file: %s\n", strerror(errno));
#else
        _gpt_clog_err("(clog): %s", message);
#endif
        if (message != msg) {
            free(message);
        }
        if (dynbuf != buf) {
            free(dynbuf);
        }
    }
}

void
gpt_clog_info(gpt_clog_t *clog,
            clog_level level,
            const char *sfile,
            int sline, 
            const char *fmt, ...)
{
    pthread_mutex_lock(&clog->mutex);
    va_list ap;
    va_start(ap, fmt);
    _gpt_clog_write(clog, level, sfile, sline, fmt, ap);
    va_end(ap);
    pthread_mutex_unlock(&clog->mutex);
}

/*
 * create clog object, path is log file path eg.
 * ./log, /log or ./log/
 * Create the directory if it does not exist
 */
gpt_clog_t *
gpt_clog_creat(const char *path, uint64_t maxsize) {
    gpt_clog_t *logger = NULL;

    if (path == NULL) {
        assert(0 && "(clog): log filename equal NULL");
        return 0;
    }

    if (strlen(path) > MAXFILELEN) {
        assert(0 && "(clog): filename exceeds the maximum number of characters");
        return 0;
    }

    // Judgment is not a directory
    if (mkdirp(path, 0777) == -1) {
        _gpt_clog_err("(clog): mkdir() failed. %s %s\n", path, strerror(errno));
        goto err;
    }

    if ((logger = (gpt_clog_t *)calloc(1, sizeof(*logger))) == NULL)
        goto err;
    _gpt_clog_filename(logger->filename, sizeof(logger->filename), path);

    logger->level = CLOG_DEBUG;
    logger->tfmt = gf_timefmt_bdT;
    strncpy(logger->fmt, CLOG_DEFAULT_FORMAT, sizeof(logger->fmt));
    /*
     * A mutex variable is represented by the pthread_mutex_t data type. Before we
     * can use a mutex variable, we must first initialize it by either setting it to the constant
     * PTHREAD_MUTEX_INITIALIZER (for statically allocated mutexes only) or calling
     * pthread_mutex_init.
     */
    if (pthread_mutex_init(&logger->mutex, NULL) != 0)
        goto err;

    /*
     * The open() system call opens the file specified by pathname.  
     * If the specified file does not exist, it may op‐tionally 
     * (if O_CREAT is specified in flags) be created by open().
     */
    if((logger->fd = open(logger->filename, O_CREAT | O_WRONLY | O_APPEND, 0666)) == -1) {
        _gpt_clog_err("(clog): open() %s function failed.\n", logger->filename, strerror(errno));
        goto err;
    }

    return logger;
err:
    if (logger)
        free(logger);
    return NULL;
}

void
gpt_clog_close(gpt_clog_t *clog) {
    if (clog == NULL)
        return;
    
    pthread_mutex_lock(&clog->mutex);
    pthread_mutex_unlock(&clog->mutex);
    pthread_mutex_destroy(&clog->mutex);

    close(clog->fd);
    free(clog);
}