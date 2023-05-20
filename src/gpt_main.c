/*
 * Copyright (c) 2023-2023 HXU-YanRuiBing <772166784@qq.com> All rights reserved.
 */
#include <gpt_config.h>

static const char usage[]
    = "\n"
	  " /----------------------------------------------------------------------------\\\n"
	  " |                                                                            |\n"
	  " |  cgpt  -- Cgpt Open SYnthesis Suite                                        |\n"
	  " |                                                                            |\n"
	  " |  Copyright (C) 2023 - 2023  Yan Rui Bing <yanruibing@gmail.com>            |\n"
	  " |                                                                            |\n"
	  " |  Permission to use, copy, modify, and/or distribute this software for any  |\n"
	  " |  purpose with or without fee is hereby granted, provided that the above    |\n"
	  " |  copyright notice and this permission notice appear in all copies.         |\n"
	  " |                                                                            |\n"
	  " |  THE SOFTWARE IS PROVIDED \"AS IS\" AND THE AUTHOR DISCLAIMS ALL WARRANTIES  |\n"
	  " |  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF          |\n"
	  " |  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR   |\n"
	  " |  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    |\n"
	  " |  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN     |\n"
	  " |  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF   |\n"
	  " |  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.            |\n"
	  " |                                                                            |\n"
      " |                                                       version : 1.0.1      |\n"
	  " \\----------------------------------------------------------------------------/\n"
	  "\n\n";

static const char helpusage[]
	= "\n"
	  "  cgpt - (C) 2023-2023 By hxuyrb\n"
	  "\n"
	  "  usage: cgpt [options]\n"
	  "\n"
	  "  options:\n"
	  "\n"
	  "      -x <proxy> : set proxy address (eg. @127.0.0.1:6666)\n"
	  "      -k <key>   : API key generated by the ChatGPT official website.\n"
	  "      -f <file>  : JSON configuration file settings.\n"
      "      --url      : http URL (eg. https://api.openai.com/v1/chat/completions).\n"
	  "      --timeout  : Set curl connection timeout (default 10).\n"
	  "      -v         : Displays version.\n"
	  "      -h <help>  : Displays this usage screen.\n"
	  "\n";

char *gpt_cmd_prompt = NULL;
static gpt_option_t opt = {
    .auth = NULL,
    .head = NULL,
    .proxy = NULL,
    .url = NULL,
    .timeout = 0,
    .clog = NULL,
};

static void gpt_do_completion(char const *prefix, linenoiseCompletions* lc);
static char *gpt_do_hints(const char *buf, int *color, int *bold);
static char *gpt_request_data(char **request, int n);
static void gpt_request_free(gpt_request_t *rq, int n);
static char *gpt_request_cmd(const char *content);
static FILE *gpt_request_send(const char *cmdline);
static void gpt_response_parser(FILE *fp);

void
gpt_console_loop() {
    int              status;
    char            *line = NULL;
    FILE            *fp;
    gpt_object_t    *oj;
    gpt_cmd_prompt   = gpt_prompt;
    
    /* Parse options, with we enable multi line editing. */
    linenoiseSetMultiLine(1);
    /* Set the completion callback. This will be called every time the
     * user uses the <tab> key. */
    linenoiseSetCompletionCallback(gpt_do_completion);
    linenoiseSetHintsCallback(gpt_do_hints);

    /* Load history from file. The history file is just a plain text file
     * where entries are separated by newlines. */
    linenoiseHistoryLoad("history.txt"); /* Load the history at startup */

    /* Now this is the main loop of the typical linenoise-based application.
     * The call to linenoise() will block as long as the user types something
     * and presses enter.
     *
     * The typed string is returned as a malloc() allocated string by
     * linenoise, so the user needs to free() it. */
    while (1) {
        line = linenoise(gpt_cmd_prompt);
        if (line == NULL)
            break;

        if (line[0] != '\0' && line[0] != '/') {
            char *str = gpt_request_data(&line, 1);
            // build command
            char *cmd = gpt_request_cmd(str);
            free(str);
            // Start sending the request and parse the data
            if (cmd != NULL) {
                fp = gpt_request_send(cmd);
                //fp = fopen("log.json", "rb");
                if (fp != NULL) {
                    gpt_response_parser(fp);
                    free(cmd);
                    clearerr(fp);

                    if ((status = pclose(fp)) == -1) {
                        printf("(clog): pclose %s\n", strerror(errno));
                    }
                    
                    if (WIFEXITED(status)) {
                        //printf("(clog): Exited with status %d\n", WEXITSTATUS(status));
                    } else {
                        printf("(clog): Exited abnormally.\n");
                    }
                }
            }
        } else {

        }
        linenoiseFree(line);
    }
    linenoiseHistorySave("history.txt");
}

static void  
gpt_do_completion(char const *prefix, linenoiseCompletions *lc) {

}

static char *
gpt_do_hints(const char *buf, int *color, int *bold) {
    if (!strcasecmp(buf,"hello")) {
        *color = 35;
        *bold = 0;
        return " World";
    }
    return NULL;
}

/*
 * Initialize fixed parameters:
 * -x "@127.0.0.1:1080"
 * -H "Content-Type: application/json"
 * -H "Authorization: Bearer @chatgpt-key"
 */
static void 
gpt_request_init(const gpt_jfile_t *jf) {
    if (jf && jf->key) {
        opt.auth = strdup(jf->key);
    } else {
        if (opt.auth == NULL)
            opt.auth = strdup(getenv("CHATGPT_KEY"));
    }

    if (jf && jf->head) {
        opt.head = strdup(jf->head);
    } else {
        if (opt.head == NULL)
            opt.head = strdup(GPT_HEAD);
    }

    if (jf && jf->url) {
        opt.url = strdup(jf->url);
    } else {
        if (opt.url == NULL)
            opt.url = strdup(GPT_URL);
    }
        
    
    if (jf && jf->proxy) {
        opt.proxy = strdup(jf->proxy);
    } else {
        if (opt.proxy == NULL)
            opt.proxy = strdup(GPT_PROXY);
    }

    if (jf && jf->timeout != 0) {
        opt.timeout = jf->timeout;
    } else {
        if (opt.timeout == 0)
            opt.timeout = 10;
    }
}

static void
gpt_request_clear() {
    if (opt.head != NULL) free(opt.head);
    if (opt.auth != NULL) free(opt.auth);
    if (opt.url != NULL) free(opt.url);
    if (opt.proxy != NULL) free(opt.proxy);
}

static char *
gpt_request_data(char **request, int n) {
    int             i;
    char            *data;
    gpt_message_t   *gmsg;
    gpt_request_t   param;

    strncpy(param.model, GPT_MODEL, sizeof(param.model));
    param.temperature = 0.7;
    /*
     * Designing n+1 requests is to make the last one empty, 
     * so that it is convenient to confirm the boundary when 
     * traversing
     */
    param.msg = malloc(n * sizeof(*param.msg));
    memset(param.msg, 0, n * sizeof(*param.msg));
    gmsg = param.msg;
    for (i = 0; i < n; i++) {
        gmsg->content = strdup(*(request + i));
        strncpy(gmsg->role, "user", sizeof(gmsg->role));
        gmsg++;
    }
    /*
     * Get request packet string
     */
    data = gpt_json_data(&param, n);
    /*
     * The  strdup() function returns a pointer to a new string 
     * which is a duplicate of the string s.  Memory for the
     * new string is obtained with malloc(3), and can be freed with free(3).
     */
    gpt_request_free(&param, n);
    return data;
}

static void
gpt_request_free(gpt_request_t *rq, int n) {
    gpt_message_t   *gmsg;

    gmsg = rq->msg;
    for (int i = 0; i < n; i++) {
        free(gmsg->content);
    }
    free(gmsg);
}

/*
 * Build the following request command:
 * curl -s -x "@127.0.0.1:1080" "https://api.openai.com/v1/chat/completions"      \
 * -H "Content-Type: application/json"                                            \
 * -H "Authorization: Bearer sk-"                                                 \
 * -d '{
 *   "model": "gpt-3.5-turbo",
 *    "messages": [{"role": "user", "content": "hello world"}, 
 *                  {"role": "user", "content": "hello ai"},],
 *    "temperature": 0.7}'
 * The returned command needs to call free to release after use.
 */
static char *
gpt_request_cmd(const char *content) {
    char   *cmdline;
    int     len1, len2;

    cmdline = (char *)malloc(MAXLINE);
    memset(cmdline, 0, MAXLINE);

    strcat(cmdline, "curl");
    strcat(cmdline, " --insecure -s --show-error ");
    if (opt.proxy != NULL) {
        strcat(cmdline, " -x ");
        strcat(cmdline, opt.proxy);
    }
    strcat(cmdline, " ");
    strcat(cmdline, opt.url);
    if (opt.head != NULL) {
        strcat(cmdline, " -H ");
        strcat(cmdline, opt.head);
    }
    strcat(cmdline, " -H ");
    strcat(cmdline, opt.auth);
    strcat(cmdline, " -d ");
    strcat(cmdline, "'");

    len1 = strlen(cmdline);
    len2 = strlen(content);
    if (len1 + len2 >= MAXLINE) {
        cmdline = realloc(cmdline, len1 + len2 + 10);
        if (cmdline == NULL)
            return NULL;
    }
    strcat(cmdline, content);
    strcat(cmdline, "'");
    //GCLOG_ERROR(opt.clog, "%s", cmdline);
    return cmdline;
}

/*
 * Send request, parse data
 */
static FILE *
gpt_request_send(const char *cmdline) {
    FILE   *fp;

    /*
     * The  popen() function opens a process by creating a pipe, 
     * forking, and invoking the shell.  Since a
     * pipe is by definition unidirectional, the type argument may
     * specify only reading  or  writing,  not
     * both; the resulting stream is correspondingly read-only or write-only.
     */
    if ((fp = popen(cmdline, "r")) != NULL)
        return fp;
    return NULL;
}

/*
 * Read data from the stream and save it in buf,
 * and call free to release it after use
 * return NULL after 3 consecutive timeouts without data returned
 */
static int
gpt_respond_buf(FILE *fp, char **buf) {
    // Setup select() parameters
    fd_set          rfds;
    struct timeval  tv;
    int             retval;
    int             count = 0;

    FILE           *fm;
    char           *bf = NULL;
    char           *line = NULL;
    size_t          len;
    size_t          size = 0;

    /*
     * The function ferror() tests the error indicator for the stream pointed to by stream,
     * returning nonzero if it  is set.  The error indicator can be 
     * reset only by the clearerr() function.
     */
    if (ferror(fp)) {
        GCLOG_ERROR(opt.clog, "%d,%s", getpid(), "ferror true.");
        goto err;
    }

    // Watch stdin (fd 0) to see when it has input.
    FD_ZERO(&rfds);
    FD_SET(fileno(fp), &rfds);

    // Wait up to 5 seconds.
    tv.tv_sec = 15;
    tv.tv_usec = 0;

    // Wait until file is ready for reading
    while (1) {
        retval = select(fileno(fp) + 1, &rfds, NULL, NULL, &tv);
        if (retval == -1) {
            perror("select()");
            exit(1);
        } else if (retval == 0) {
            GCLOG_INFO(opt.clog, "%d,%s %d", getpid(), "Timeout reached", count);
            if (++count == 3)
                return 0;
            /*
             * If the timeout count is less than 3, continue to extend the waiting time.
             */
            FD_ZERO(&rfds);
            FD_SET(fileno(fp), &rfds);
            tv.tv_sec = count * 25;
            tv.tv_usec = 0;
            continue;
        } else {
            if (FD_ISSET(fileno(fp), &rfds))
                break;
        }
    }

    /*
     * The open_memstream function is a system call used in the C programming language
     * to create a stream that can write data to a memory buffer. It takes two arguments:
     * a pointer to a character array, which will be the buffer to write the data into,
     * and a pointer to an integer, which will be used to store the size of the data
     * written to the buffer.
     */
    fm = open_memstream(buf, &size);
    while (getline(&line, &len, fp) != -1) {
        fputs(line, fm);
    }
    fflush(fm);
    free(line);
    fclose(fm);
    
    return 0;
err:
    return -1;
}

static inline void
__gpt_print_data(char *s) {
    printf("\n");
    while (*s != '\0') {
        usleep(50000);
        printf("%c", *s);
        fflush(stdout);
        s++;
    }
    printf("\n\n");
}

static void
gpt_response_parser(FILE *fp) {
    char           *buf = NULL, *s;
    cJSON          *root;
    int             status = 0;

    status = gpt_respond_buf(fp, &buf);
    if (buf == NULL || status == -1)
        return;
    
    status = gpt_json_root(buf, &root);

    if (status == 0) {
        gpt_object_t   *obj = NULL;

        obj = gpt_json_parse(buf);
        for (int i = 0; i < obj->choices_num; i++) {
            gpt_choice_t *t = obj->choices + i;
            s = t->msg.content;
            __gpt_print_data(s);
        }
        gpt_json_free(obj);
    } else {
        gpt_error_t    *err;

        err = gpt_json_error(buf);
        s = err->message;
        __gpt_print_data(s);
        gpt_json_error_free(err);
    }
    cJSON_Delete(root);
    /*
     * Read data from the stream and save it in buf,
     * and call free to release it after use
     */
    free(buf);
    return;
}

static gpt_jfile_t *
gpt_file_option() {
    char *jf = NULL;
    gpt_jfile_t *gjf = NULL;
    // first free option's old value
    gpt_request_clear();
    jf = readfile(opt.jfile);

    if (jf) {
        gjf = gpt_json_file(jf);
        free(jf);
    }

    return gjf;
}


int main(int argc, char *argv[]) {
    int c;
    gpt_object_t *oj;
    gpt_jfile_t *jf = NULL;

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"proxy",   required_argument, 0,  'x' },
            {"key",     required_argument, 0,  'k' },
            {"file",    required_argument, 0,  'f' },
            {"url",     required_argument, 0,   0  },
            {"timeout", required_argument, 0,   0  },
            {"help",    no_argument,       0,  'h' },
            {0,         0,                 0,   0  }
        };

        c = getopt_long(argc, argv, "x:k:f:hv", long_options, &option_index);
        if (c == -1)
            break;
        
        switch (c) {
        case 0:
        {
            // set time out
            if (option_index == 4) {
                if (optarg)
                    opt.timeout = strtol(optarg, NULL, 10); 
            }
            // set url
            if (option_index == 3) {
                if (optarg) {
                    opt.url = strdup(optarg);
                }
            }
            break;
        }
        case 'x':
            if(optarg) {
                opt.proxy = strdup(optarg);
            }
            break;
        case 'k':
            if(optarg) {
                opt.auth = strdup(optarg);
            }
            break;
        case 'f':
            if(optarg) {
                strncpy(opt.jfile, optarg, sizeof(opt.jfile));
                jf = gpt_file_option();
            }
            break;
        case 'v':
            printf("%s %s\n", argv[0]+2, GPT_VERSION);
            exit(EXIT_SUCCESS);
            break;
        case 'h':
            printf("%s", helpusage);
            exit(EXIT_SUCCESS);
        default:
            printf("please \"%s --help\" for help.\n", argv[0]);
            exit(EXIT_FAILURE);
            break;
        }
    }

    if (optind < argc) {
        printf("non-option ARGV-elements: ");
        while (optind < argc)
            printf("%s ", argv[optind++]);
        printf("\n");
        exit(EXIT_FAILURE);
    }
    
    gpt_request_init(jf);
    gpt_json_file_free(jf);

    printf("%s", usage);
    opt.clog = gpt_clog_creat("./logcgpt/", 1024);
    if (opt.clog == NULL) {
        return -1;
    }
    //GCLOG_INFO(opt.clog, "%d,%s", getpid(), "clog Initialization.");

    gpt_console_loop();
    gpt_request_clear();
    gpt_clog_close(opt.clog);

    return 0;
}