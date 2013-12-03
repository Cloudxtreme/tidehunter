#pragma once
#include "ngx_http_tidehunter_common.h"


typedef struct{
    ngx_array_t *head_filter_rule_a;
    ngx_array_t *body_filter_rule_a;
} ngx_http_tidehunter_main_conf_t;

typedef struct{
    ngx_array_t *head_filter_rule_a;
    ngx_array_t *body_filter_rule_a;
} ngx_http_tidehunter_loc_conf_t;

typedef struct{
    ngx_int_t body_done;
    ngx_int_t match_hit;
} ngx_http_tidehunter_ctx_t;    /* this is a ctx create by http core for every req */
