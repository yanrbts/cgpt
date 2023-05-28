/*
 * Copyright (c) 2023-2023 HXU-YanRuiBing <772166784@qq.com> All rights reserved.
 * Processes different modules of the chatgpt module in modular mode and provides 
 * unified module interfaces.
 */
#ifndef __GPT_MODULE__
#define __GPT_MODULE__

#include <gpt_config.h>

struct gpt_module_s {
    const char *name;
    const char *model;
    const char *url;
    const char *description;
    char *    (*rqfunc)(char **request, int n);
    void      (*rpfunc)(FILE *fp);
};

void gpt_module_completion();

extern gpt_module_t *gpt_modules[];

#endif
