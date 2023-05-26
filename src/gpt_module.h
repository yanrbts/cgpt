/*
 * Copyright (c) 2023-2023 HXU-YanRuiBing <772166784@qq.com> All rights reserved.
 * Processes different modules of the chatgpt module in modular mode and provides 
 * unified module interfaces.
 */
#ifndef __GPT_MODULE__
#define __GPT_MODULE__

struct gpt_module_s {
    char *name;
    char *url;
    char *description;
};

#endif
