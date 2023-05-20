/*
 * Copyright (c) 2023-2023 HXU-YanRuiBing <772166784@qq.com> All rights reserved.
 */
#ifndef __GPT_JSON__
#define __GPT_JSON__

#include <gpt_config.h>

struct usage {
    int prompt_tokens;
    int completion_tokens;
    int total_tokens;
};

struct message {
    char role[32];
    char *content;
};

/*
 * request packet
 */
struct request {
    char model[32];
    gpt_message_t *msg;
    float temperature;
};

struct choice {
    gpt_message_t msg;
    char finish_reason[10];
    int index;
};

struct object {
    char id[128];           // chatcmpl-73JBYywriRW3JO0f7NMW7jlTvY8FQ
    char object[64];        // chat.completion
    char model[32];         // gpt-3.5-turbo-0301
    uint64_t created;       // 1681023088
    gpt_usage_t  *pusage;
    gpt_choice_t *choices;
    int  choices_num;
};

struct error {
    char *message;
    char type[128];
    char param[32];
    char code[32];
};

struct jfile {
    char *key;
    char *url;
    char *head;
    char *proxy;
    int  timeout;
};

int gpt_json_root(const char *js, cJSON **root);
char *gpt_json_data(gpt_request_t *rq, int n);
gpt_object_t *gpt_json_parse(const char *js);
void gpt_json_free(gpt_object_t *obj);
gpt_error_t *gpt_json_error(const char *js);
void gpt_json_error_free(gpt_error_t *obj);
gpt_jfile_t *gpt_json_file(const char *js);
void gpt_json_file_free(gpt_jfile_t *obj);

#endif