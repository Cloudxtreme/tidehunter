#pragma once
#include<stdio.h>
#include<nginx.h>
#include<ngx_config.h>
#include<ngx_core.h>
#include<ngx_http.h>
#include<ngx_log.h>

static ngx_int_t ngx_http_test_init(ngx_conf_t *cf);

static void* ngx_http_test_create_main_conf(ngx_conf_t *cf);

static void* ngx_http_test_create_loc_conf(ngx_conf_t *cf);

static ngx_int_t ngx_http_test_rewrite_handler(ngx_http_request_t *req);

static char* ngx_http_test_test(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

typedef struct{
    ngx_str_t info_str;
} ngx_http_test_main_conf_t;

typedef struct{
    ngx_str_t info_str;
} ngx_http_test_loc_conf_t;
