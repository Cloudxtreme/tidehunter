#pragma once
#include "ngx_http_tidehunter_common.h"

#define RING_SIZE 1000

typedef struct{
    ngx_int_t hist_weight[RING_SIZE];   /* this array is a ring, the tail_pos always point to the tail */
    ngx_int_t tail_pos;
    ngx_int_t tail_weight;      /* == hist_weight[tail_pos] */
    float     stdvar;
    float     average;
} ngx_http_tidehunter_smart_t;


char *ngx_http_tidehunter_smart_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
ngx_int_t ngx_http_tidehunter_smart_test(ngx_http_request_t *req, ngx_int_t weight);
