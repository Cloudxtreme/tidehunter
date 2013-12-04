#pragma once

#include "ngx_http_tidehunter_common.h"


typedef struct{
    ngx_array_t *qstr_filter_rule_a;
    ngx_array_t *body_filter_rule_a;
    ngx_str_t qstr_rulefile;
    ngx_str_t body_rulefile;
} ngx_http_tidehunter_main_conf_t;

typedef struct{
    ngx_array_t *qstr_filter_rule_a;
    ngx_array_t *body_filter_rule_a;
} ngx_http_tidehunter_loc_conf_t;
