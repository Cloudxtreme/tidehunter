#include "ngx_http_tidehunter_module.h"

static ngx_http_module_t ngx_http_tidehunter_module_ctx = {
    NULL,
    ngx_http_test_init,

    ngx_http_test_create_main_conf,
    NULL,

    NULL,
    NULL,

    ngx_http_test_create_loc_conf,
    NULL
};

static ngx_command_t ngx_http_test_commands[] = {
    {
        ngx_string("test"),
        NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_ANY,
        ngx_http_test_test,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    ngx_null_command
};

ngx_module_t ngx_http_tidehunter_module = {
    NGX_MODULE_V1,
    &ngx_http_tidehunter_module_ctx,
    ngx_http_test_commands,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};

static ngx_int_t ngx_http_test_init(ngx_conf_t *cf){
    ngx_http_test_main_conf_t *mcf;
    /*ngx_http_test_loc_conf_t *lcf;*/
    ngx_http_core_main_conf_t *ccf;
    ngx_http_handler_pt *rewrite_handler_pt;

    ccf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_tidehunter_module);
    if(ccf == NULL || mcf == NULL){
        return NGX_ERROR;
    }
    rewrite_handler_pt = ngx_array_push(&ccf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
    if(rewrite_handler_pt == NULL){
        return NGX_ERROR;
    }
    *rewrite_handler_pt = ngx_http_test_rewrite_handler;
    return NGX_OK;
}

static void* ngx_http_test_create_main_conf(ngx_conf_t *cf){
    ngx_http_test_main_conf_t *mcf;
    mcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_test_main_conf_t));
    return mcf;
}

static void* ngx_http_test_create_loc_conf(ngx_conf_t *cf){
    ngx_http_test_loc_conf_t *lcf;
    lcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_test_loc_conf_t));
    return lcf;
}

static ngx_int_t ngx_http_test_rewrite_handler(ngx_http_request_t *req){
    fprintf(stderr, "hello world\n");
    return NGX_DECLINED;
}

static char* ngx_http_test_test(ngx_conf_t *cf, ngx_command_t *cmd, void *conf){
    return NGX_CONF_OK;
}
