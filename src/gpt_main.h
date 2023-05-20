/*
 * Copyright (c) 2023-2023 HXU-YanRuiBing <772166784@qq.com> All rights reserved.
 */
#ifndef __GPT_MAIN__
#define __GPT_MAIN__

#include <gpt_config.h>

struct gptoption {
    char *url;
    char *proxy;
    char *auth;
    char *head;
    char  jfile[PATH_MAX];
    long  timeout;
    gpt_clog_t *clog; /* If you want to log to a file in the logging module 
                       * and use the compilation option -DCLOG_OPTION at compile time.*/
};

#endif