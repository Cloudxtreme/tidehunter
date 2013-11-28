#pragma once
#include "ngx_http_tidehunter_common.h"


typedef struct{
    ngx_array_t *filter_rule_a;
} ngx_http_tidehunter_main_conf_t;

typedef struct{
    ngx_array_t *filter_rule_a;
} ngx_http_tidehunter_loc_conf_t;
