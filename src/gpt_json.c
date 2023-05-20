/*
 * Copyright (c) 2023-2023 HXU-YanRuiBing <772166784@qq.com> All rights reserved.
 */
#include <gpt_config.h>

int
gpt_json_root(const char *js, cJSON **root) {
    if (js == NULL)
        return -1;
    *root = cJSON_Parse(js);
    if (root != NULL) {
        if (cJSON_HasObjectItem(*root, "choices"))
            return 0;
        if (cJSON_HasObjectItem(*root, "error"))
            return 1;
    }
    
    return -1;
}
/*
 * Create a request json packet and convert it into a string
 */
char *
gpt_json_data(gpt_request_t *rq, int n) {
    cJSON           *root;
    cJSON           *array;
    gpt_message_t   *gmsg;
    char            *rp;

    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", rq->model);
    cJSON_AddNumberToObject(root, "temperature", rq->temperature);

    array = cJSON_CreateArray();
    gmsg = rq->msg;
    for (int i = 0; i < n; i++) {
        cJSON *msg = cJSON_CreateObject();
        cJSON_AddStringToObject(msg, "role", gmsg->role);
        cJSON_AddStringToObject(msg, "content", gmsg->content);
        cJSON_AddItemToArray(array, msg);
        gmsg++;
    }
    cJSON_AddItemToObject(root, "messages", array);
    rp = cJSON_Print(root);
    cJSON_Delete(root);
    return rp;
}

/*
 * {
  "id": "chatcmpl-749MeUvr9V1qCsrlrqgUNStsJJbXH",
  "object": "chat.completion",
  "created": 1681223664,
  "model": "gpt-3.5-turbo-0301",
  "usage": {
    "prompt_tokens": 13,
    "completion_tokens": 152,
    "total_tokens": 165
  },
  "choices": [
    {
      "message": {
        "role": "assistant",
        "content": "Analysis in C by Mark Allen Weiss."
      },
      "finish_reason": "stop",
      "index": 0
    }
  ]
}
 */
gpt_object_t *
gpt_json_parse(const char *js) {
    cJSON           *root;
    gpt_object_t    *obj = NULL;

    obj = (gpt_object_t *)malloc(sizeof(*obj));
    memset(obj, 0, sizeof(*obj));

    root = cJSON_Parse(js);
    //root = cJSON_ParseWithOpts(js + offset, NULL, require_termination);
    if (root != NULL) {
        strncpy(obj->id, cJSON_GetObjectItem(root, "id")->valuestring, sizeof(obj->id));
        strncpy(obj->object, cJSON_GetObjectItem(root, "object")->valuestring, sizeof(obj->object));
        strncpy(obj->model, cJSON_GetObjectItem(root, "model")->valuestring, sizeof(obj->model));
        obj->created = cJSON_GetObjectItem(root, "object")->valueint;

        int     size;
        cJSON  *jchoices;
        gpt_choice_t *pch;

        jchoices = cJSON_GetObjectItem(root, "choices");
        size = cJSON_GetArraySize(jchoices);

        obj->choices = (gpt_choice_t *)malloc(size * sizeof(*obj->choices));
        memset(obj->choices, 0, size * sizeof(*obj->choices));

        pch = obj->choices;
        for (int i = 0; i < size; i++) {
            cJSON       *ch, *msg, *content;
            uint64_t    len = 0;

            ch = cJSON_GetArrayItem(jchoices, i);

            strncpy(pch->finish_reason, 
                    cJSON_GetObjectItem(ch, "finish_reason")->valuestring,
                    sizeof(pch->finish_reason));
            pch->index = cJSON_GetObjectItem(ch, "index")->valueint;

            msg = cJSON_GetObjectItem(ch, "message");
            content = cJSON_GetObjectItem(msg, "content");
            len = strlen(content->valuestring);
            pch->msg.content = (char *)malloc(len + 1);
            strncpy(pch->msg.content, content->valuestring, len + 1);
            
            pch++;
            obj->choices_num++;
        }
    } else {
        printf("(cgpt): Error before: [%s]\n",cJSON_GetErrorPtr());
        return NULL;
    }
    
    if (root)
        cJSON_Delete(root);
    return obj;
}

void 
gpt_json_free(gpt_object_t *obj) {
    if (obj != NULL && obj->choices != NULL) {
        for (int i = 0; i < obj->choices_num; i++) {
            gpt_choice_t *ch = obj->choices + i;
            free(ch->msg.content);
        }
        free(obj->choices);
        free(obj);
    } 
}

gpt_error_t *
gpt_json_error(const char *js) {
    cJSON          *root;
    gpt_error_t    *obj = NULL;

    obj = (gpt_error_t *)calloc(1, sizeof(*obj));

    root = cJSON_Parse(js);
    if (root != NULL) {
        cJSON  *error = NULL, *param = NULL, *code = NULL;

        error = cJSON_GetObjectItem(root, "error");

        obj->message = (char *)calloc(1, sizeof(*obj->message));
        strncpy(obj->message, 
                cJSON_GetObjectItem(error, "message")->valuestring,
                sizeof(obj->message));
        strncpy(obj->type, 
                cJSON_GetObjectItem(error, "type")->valuestring,
                sizeof(obj->type));
        
        param = cJSON_GetObjectItem(error, "param");
        if (cJSON_IsNull(param)) {
            strncpy(obj->type, "null", sizeof(obj->param));
        } else {
            strncpy(obj->param, 
                cJSON_GetObjectItem(error, "param")->valuestring,
                sizeof(obj->param));
        }

        code = cJSON_GetObjectItem(error, "code");
        if (cJSON_IsNull(code)) {
            strncpy(obj->code, "null", sizeof(obj->code));
        } else {
            strncpy(obj->code, 
                cJSON_GetObjectItem(error, "code")->valuestring,
                sizeof(obj->code));
        }
    }
    return obj;
}

void
gpt_json_error_free(gpt_error_t *obj) {
    if (obj != NULL) {
        free(obj->message);
        free(obj);
    }
}

gpt_jfile_t *
gpt_json_file(const char *js) {
    cJSON       *root;
    gpt_jfile_t *obj = NULL;

    obj = (gpt_jfile_t *)calloc(1, sizeof(*obj));

    root = cJSON_Parse(js);
    if (root != NULL) {
        obj->key = strdup(cJSON_GetObjectItem(root, "key")->valuestring);
        obj->url = strdup(cJSON_GetObjectItem(root, "url")->valuestring);
        obj->head = strdup(cJSON_GetObjectItem(root, "head")->valuestring);
        obj->proxy = strdup(cJSON_GetObjectItem(root, "proxy")->valuestring);
        obj->timeout = cJSON_GetObjectItem(root, "timeout")->valueint;
    }

    if (root)
        cJSON_Delete(root);

    return obj;
}

void
gpt_json_file_free(gpt_jfile_t *obj) {
    if (obj != NULL) {
        free(obj->key);
        free(obj->url);
        free(obj->head);
        free(obj->proxy);
        free(obj);
    }
}