#pragma once

#include "ngx_http_tidehunter_common.h"
#include "ngx_http_tidehunter_module.h"


typedef struct{
    ngx_str_t msg;
    ngx_str_t id;
    ngx_int_t weight;
    /* option for string match: exact or regex. */
    ngx_http_tidehunter_match_option_e match_opt;
    ngx_str_t exact_str;
    ngx_regex_compile_t *compile_regex;
} ngx_http_tidehunter_filter_rule_t;


ngx_int_t ngx_http_tidehunter_filter_qstr(ngx_http_request_t *req,
                                          ngx_array_t *opt);

ngx_int_t ngx_http_tidehunter_filter_body(ngx_http_request_t *req,
                                          ngx_array_t *opt);

ngx_int_t ngx_http_tidehunter_filter_uri(ngx_http_request_t *req,
                                         ngx_array_t *rule_a);


char *ngx_http_tidehunter_filter_qstr_init(ngx_conf_t *cf,
                                          ngx_command_t *cmd,
                                          void *conf);

char *ngx_http_tidehunter_filter_body_init(ngx_conf_t *cf,
                                          ngx_command_t *cmd,
                                          void *conf);

char *ngx_http_tidehunter_filter_uri_init(ngx_conf_t *cf,
                                          ngx_command_t *cmd,
                                          void *conf);
