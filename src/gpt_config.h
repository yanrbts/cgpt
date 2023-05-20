/*
 * Copyright (c) 2023-2023 HXU-YanRuiBing <772166784@qq.com> All rights reserved.
 */
#ifndef __GPT_CONFIG__
#define __GPT_CONFIG__

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <cJSON.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <gpt_linenoise.h>
#include <gpt_common.h>

typedef struct gptoption    gpt_option_t;
typedef struct usage        gpt_usage_t;
typedef struct message      gpt_message_t;
typedef struct request      gpt_request_t;
typedef struct choice       gpt_choice_t;
typedef struct object       gpt_object_t;
typedef struct error        gpt_error_t;
typedef struct clog         gpt_clog_t;
typedef struct jfile        gpt_jfile_t;

#include <gpt_json.h>
#include <gpt_log.h>
#include <gpt_main.h>

#define gpt_prompt      "cgpt>"
#define GPT_URL         "\"https://api.openai.com/v1/chat/completions\""
#define GPT_PROXY       "\"@127.0.0.1:9666\""
#define GPT_HEAD        "\"Content-Type: application/json\""
#define GPT_MODEL       "gpt-3.5-turbo"
#define MAXLINE         1024
#define GPT_MAXBUF      4096
#define GPT_VERSION     "1.0.1"

#endif