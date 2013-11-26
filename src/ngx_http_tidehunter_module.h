#pragma once
#include "ngx_http_tidehunter_common.h"

static ngx_int_t ngx_http_tidehunter_init(ngx_conf_t *cf);

static void* ngx_http_tidehunter_create_main_conf(ngx_conf_t *cf);

static void* ngx_http_tidehunter_create_loc_conf(ngx_conf_t *cf);

static ngx_int_t ngx_http_tidehunter_rewrite_handler(ngx_http_request_t *req);

static char* ngx_http_tidehunter_test(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

typedef struct{
    ngx_str_t info_str;
} ngx_http_tidehunter_main_conf_t;

typedef struct{
    ngx_str_t info_str;
} ngx_http_tidehunter_loc_conf_t;
