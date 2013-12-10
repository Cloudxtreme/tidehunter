#pragma once

#include "ngx_http_tidehunter_common.h"
#include "ngx_http_tidehunter_module.h"


typedef struct{
    /* option for filter func ptr */
    ngx_http_tidehunter_match_option_e match_opt;
    ngx_str_t exact_str;
    ngx_regex_compile_t *compile_regex;
} ngx_http_tidehunter_filter_option_t;


typedef struct{
    ngx_str_t msg;
    ngx_str_t id;
    ngx_int_t weight;
    ngx_http_tidehunter_filter_option_t opt;
} ngx_http_tidehunter_filter_rule_t;


ngx_int_t ngx_http_tidehunter_filter_qstr(ngx_http_request_t *req,
                                          ngx_array_t *opt);

ngx_int_t ngx_http_tidehunter_filter_body(ngx_http_request_t *req,
                                          ngx_array_t *opt);

ngx_int_t ngx_http_tidehunter_filter_init_rule(ngx_http_tidehunter_main_conf_t *mcf,
                                               ngx_http_tidehunter_filter_type_e filter_type,
                                               ngx_pool_t *pool);
