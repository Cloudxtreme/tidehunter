#pragma once

#include "ngx_http_tidehunter_common.h"
#include "ngx_http_tidehunter_parse.h"
#include "ngx_http_tidehunter_module.h"

/*
the filter_* functions should all be implemented like:
   int (*filter)(ngx_http_request_t *req,
                 ngx_http_tidehunter_filter_options_t *opt);
so to make it possible that filter could be implemented outside.
*/

typedef enum {
    // MO_* is [M]atch [O]ption
    MO_EXACT_MATCH,
    MO_REG_MATCH,
    MO_EXACT_MATCH_IGNORE_CASE
} ngx_http_tidehunter_match_option_e;

typedef struct{
    ngx_http_tidehunter_match_option_e match_opt;
    ngx_str_t exact_str;
    ngx_regex_compile_t *compile_regex;
} ngx_http_tidehunter_filter_option_t;

typedef struct{
    ngx_str_t msg;
    ngx_str_t id;
    ngx_int_t weight;
    ngx_int_t (*filter)(ngx_http_request_t *req,
                        ngx_http_tidehunter_filter_option_t *opt); /* a filter func ptr */
    ngx_http_tidehunter_filter_option_t opt;                       /* option for filter func ptr */
} ngx_http_tidehunter_filter_rule_t;

ngx_int_t ngx_http_tidehunter_filter_qstr(ngx_http_request_t *req,
                                          ngx_http_tidehunter_filter_option_t *opt);

ngx_int_t ngx_http_tidehunter_filter_init_rule(ngx_http_tidehunter_main_conf_t *mcf,
                                               ngx_pool_t *pool);

