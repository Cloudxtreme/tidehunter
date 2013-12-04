#pragma once

#include "ngx_http_tidehunter_common.h"
#include "ngx_http_tidehunter_const.h"


typedef struct{
    ngx_array_t *filter_rule_a[FT_TOTAL];
    ngx_str_t rulefile[FT_TOTAL];
} ngx_http_tidehunter_main_conf_t;

typedef struct{
    ngx_array_t *qstr_filter_rule_a;
    ngx_array_t *body_filter_rule_a;
} ngx_http_tidehunter_loc_conf_t;
