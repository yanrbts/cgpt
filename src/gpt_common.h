/*
 * Copyright (c) 2023-2023 HXU-YanRuiBing <772166784@qq.com> All rights reserved.
 */
#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <sys/stat.h>

typedef enum {
    gf_timefmt_default = 0,
    gf_timefmt_FT = 0, /* YYYY-MM-DD hh:mm:ss */
    gf_timefmt_Ymd_T,  /* YYYY/MM-DD-hh:mm:ss */
    gf_timefmt_bdT,    /* MMM DD hh:mm:ss */
    gf_timefmt_F_HMS,  /* YYYY-MM-DD hhmmss */
    gf_timefmt_dirent,
    gf_timefmt_day,    /* YYYY-MM-DD */
    gf_timefmt_s,
    gf_timefmt_last
} gf_timefmts;

char *get_time_fmt(char *dst, size_t sz_dst, time_t utime, unsigned int fmt);
/*
 * Modify the irregular multi-level directory, 
 * for example: /b/////c////d will be modified 
 * as a legal multi-level directory: /b/c/d, 
 * because the strdup function is used, 
 * it returns after use Need to call free to release the value
 */
char *path_normalize(const char *path);
/*
 * Create a multi-level directory at one time, 
 * such as /b/v/c/b, similar to mkdir -p /root/abc/efg, 
 * return 0 if successful, otherwise return -1
 */
int mkdirp(const char *, mode_t );
/*
 * HTTP response header and data are received, 
 * the status code is returned, and the data is 
 * stored to facilitate different data parsing.
 * Note that data needs to be free after use.
 */
int get_http_data(FILE *fp, char **data);

/*
 * Read the content of the file, then return it. 
 * If unsuccessful, return NULL. After using, 
 * the return value needs to be released
 */
char *readfile(const char *file);

#endif