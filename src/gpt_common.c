/*
 * Copyright (c) 2023-2023 HXU-YanRuiBing <772166784@qq.com> All rights reserved.
 */
#include <gpt_config.h>

#define GF_PRI_SUSECONDS "06ld"
#ifdef _WIN32
#define PATH_SEPARATOR   '\\'
#else
#define PATH_SEPARATOR   '/'
#endif

static const char *__gf_timefmts[] = {
    "%F %T",
    "%Y/%m/%d-%T",
    "%b %d %T",
    "%F %H%M%S",
    "%Y-%m-%d-%T",
    "%Y-%m-%d",
    "%s",
};

static const char *__gf_zerotimes[] = {
    "0000-00-00 00:00:00",
    "0000/00/00-00:00:00",
    "xxx 00 00:00:00",
    "0000-00-00 000000", 
    "0000-00-00-00:00:00",
    "0000-00-00",
    "0",
};

static void
_gf_timestuff(const char ***fmts, const char ***zeros)
{
    *fmts = __gf_timefmts;
    *zeros = __gf_zerotimes;
}

/*
 * return address of value equel dst
 */
static inline char *
gf_time_fmt_tv(char *dst, size_t sz_dst, struct timeval *tv, unsigned int fmt)
{
    static gf_timefmts timefmt_last = (gf_timefmts)-1;
    static const char **fmts;
    static const char **zeros;
    struct tm tm, *res;
    int len = 0;
    int pos = 0;

    if (timefmt_last == ((gf_timefmts)-1)) {
        _gf_timestuff(&fmts, &zeros);
        timefmt_last = gf_timefmt_last;
    }
    if (timefmt_last <= fmt) {
        fmt = gf_timefmt_default;
    }

    /*
     * localtime() functions all take an argument of data  type  time_t,  which  represents
     * calendar  time.  When interpreted as an absolute time value, it represents the 
     * number of seconds elapsed since the Epoch, 1970-01-01 00:00:00 +0000 (UTC).
     * localtime_r thread safe function
     */
    res = localtime_r(&tv->tv_sec, &tm);
    if (tv->tv_sec && (res != NULL)) {
        len = strftime(dst, sz_dst, fmts[fmt], &tm);
        if (len == 0)
            return dst;
        pos += len;
        if (tv->tv_usec >= 0) {
            len = snprintf(dst + pos, sz_dst - pos, ".%" GF_PRI_SUSECONDS,
                           tv->tv_usec);
            if (len >= sz_dst - pos)
                return dst;
            pos += len;
        }
        strftime(dst + pos, sz_dst - pos, " %z", &tm);
    } else {
        strncpy(dst, "N/A", sz_dst);
    }
    return dst;
}

char *
get_time_fmt(char *dst, size_t sz_dst, time_t utime, unsigned int fmt)
{
    struct timeval tv = {utime, -1};
    return gf_time_fmt_tv(dst, sz_dst, &tv, fmt);
}

/*
 * Modify the irregular multi-level directory, 
 * for example: /b/////c////d will be modified 
 * as a legal multi-level directory: /b/c/d, 
 * because the strdup function is used, 
 * it returns after use Need to call free to release the value
 */
char *
path_normalize(const char *path) {
    if (!path)
        return NULL;

    char *copy = strdup(path);
    if (NULL == copy)
        return NULL;

    char *ptr = copy;
    for (int i = 0; copy[i]; i++) {
        *ptr++ = path[i];
        if ('/' == path[i]) {
            i++;
            while ('/' == path[i])
                i++;
            i--;
        }
    }
    *ptr = '\0';
    return copy;
}

/*
 * Create a multi-level directory at one time, 
 * such as /b/v/c/b, similar to mkdir -p /root/abc/efg, 
 * return 0 if successful, otherwise return -1
 */
int
mkdirp(const char *path, mode_t mode) {
    char *pathname = NULL;
    char *parent = NULL;
    int rc = 0;

    if (path == NULL)
        return -1;
    pathname = path_normalize(path);
    if (pathname == NULL)
        goto fail;
    
    parent = strdup(pathname);
    if (NULL == parent)
        goto fail;

    char *p = parent + strlen(parent);
    while (PATH_SEPARATOR != *p && p != parent) {
        p--;
    }
    *p = '\0';

    // make parent dir
    if (p != parent && 0 != mkdirp(parent, mode))
        goto fail;
    free(parent);

    // make this one if parent has been made
#ifdef _WIN32
    rc = mkdir(pathname);
#else
    rc = mkdir(pathname, mode);
#endif
    /*
     * The  strdup() function returns a pointer to a new string which is 
     * a duplicate of the string s.  Memory for the
     * new string is obtained with malloc(3), and can be freed with free(3).
     */
    free(pathname);

    return 0 == rc || EEXIST == errno ? 0 : -1;

fail:
    free(pathname);
    free(parent);
    return -1;
}

/*
 * HTTP response header and data are received, 
 * the status code is returned, and the data is 
 * stored to facilitate different data parsing.
 * Note that data needs to be free after use.
 */
int
get_http_data(FILE *fp, char **data) {
    int     status_code = 0;
    int     header_read = 1;
    int     response_body_length = 0;
    char    response[4096];
    char   *response_body = NULL;
    
    /* Read the response */
    while (fgets(response, sizeof(response), fp) != NULL) {
        /* Look for the status code in the response */
        if (strstr(response, "HTTP/2 ") != NULL) {
            char *tmp;
            tmp = strtok(response, " ");
            tmp = strtok(NULL, " ");
            status_code = atoi(tmp);
        }
        /* Look for the response header */
        if (header_read && (strcmp(response, "\r\n") == 0)) {
            /* Set flag to indicate header has been read */
            header_read = 0;
        } else if (!header_read) {
            /* Save the response body */
            int response_line_length = strlen(response);
            response_body = realloc(response_body, response_body_length + response_line_length);
            if (response_body == NULL) {
                fprintf(stderr, "Error allocating memory\n");
                return 0;
            }
            memcpy(response_body + response_body_length, response, response_line_length);
            response_body_length += response_line_length;
        }
    }

    /* Null-terminate the response body */
    response_body = realloc(response_body, response_body_length + 1);
    if (response_body == NULL) {
        fprintf(stderr, "Error allocating memory\n");
    }
    response_body[response_body_length] = '\0';

    *data = response_body;

    return status_code;
}

/*
 * Read the content of the file, then return it. 
 * If unsuccessful, return NULL. After using, 
 * the return value needs to be released
 */
char *
readfile(const char *file) {
    FILE    *fp = NULL;
    long     length = 0;
    char    *content = NULL;
    size_t   size = 0;

    /* open in read binary mode */
    if ((fp = fopen(file, "rb")) == NULL)
        goto cleanup;

    if ((length = ftell(fp)) < 0)
        goto cleanup;

    if (fseek(fp, 0, SEEK_SET) != 0)
        goto cleanup;

    /* allocate content buffer */
    content = (char *)malloc((size_t)length + 1);
    if (content == NULL) {
        goto cleanup;
    }

    /* read the file into memory */
    size = fread(content, sizeof(char), (size_t)length, fp);
    if ((long)size != length) {
        free(content);
        content = NULL;
        goto cleanup;
    }
    content[size] = '\0';

cleanup:
    if (fp != NULL)
        fclose(fp);

    return content;
}