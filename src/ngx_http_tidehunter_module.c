#include "ngx_http_tidehunter_module.h"
#include "ngx_http_tidehunter_parse.h"
#include "ngx_http_tidehunter_filter.h"

#include "ngx_http_tidehunter_debug.h"

static ngx_int_t ngx_http_tidehunter_init(ngx_conf_t *cf);
static void* ngx_http_tidehunter_create_main_conf(ngx_conf_t *cf);
static void* ngx_http_tidehunter_create_loc_conf(ngx_conf_t *cf);
static ngx_int_t ngx_http_tidehunter_rewrite_handler(ngx_http_request_t *req);
static ngx_int_t ngx_http_tidehunter_rewrite_handler_body(ngx_http_request_t *req);
static void ngx_http_tidehunter_req_body_cb(ngx_http_request_t *req);
static char* ngx_http_tidehunter_test(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);


static ngx_http_module_t ngx_http_tidehunter_module_ctx = {
    NULL,
    ngx_http_tidehunter_init,

    ngx_http_tidehunter_create_main_conf,
    NULL,

    NULL,
    NULL,

    ngx_http_tidehunter_create_loc_conf,
    NULL
};

static ngx_command_t ngx_http_tidehunter_commands[] = {
    {
        ngx_string("test"),
        NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_ANY,
        ngx_http_tidehunter_test,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    ngx_null_command
};

ngx_module_t ngx_http_tidehunter_module = {
    NGX_MODULE_V1,
    &ngx_http_tidehunter_module_ctx,
    ngx_http_tidehunter_commands,
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

static ngx_int_t ngx_http_tidehunter_init(ngx_conf_t *cf){
    ngx_http_tidehunter_main_conf_t *mcf;
    /*ngx_http_tidehunter_loc_conf_t *lcf;*/
    ngx_http_core_main_conf_t *ccf;
    ngx_http_handler_pt *rewrite_handler_pt;

    ccf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_tidehunter_module);
    if(ccf == NULL || mcf == NULL){
        return NGX_ERROR;
    }

    /* initialize the filter rule */
    ngx_http_tidehunter_filter_init_rule(mcf, cf->pool);

    /* initialize the REWRITE handler */
    rewrite_handler_pt = ngx_array_push(&ccf->phases[NGX_HTTP_REWRITE_PHASE].handlers); /* why REWIRTE PHASE? */
    if(rewrite_handler_pt == NULL){
        return NGX_ERROR;
    }
    *rewrite_handler_pt = ngx_http_tidehunter_rewrite_handler;

    /* initialize the handler for request body */
    rewrite_handler_pt = ngx_array_push(&ccf->phases[NGX_HTTP_REWRITE_PHASE].handlers); /* why REWIRTE PHASE? */
    if(rewrite_handler_pt == NULL){
        return NGX_ERROR;
    }
    *rewrite_handler_pt = ngx_http_tidehunter_rewrite_handler_body;
    return NGX_OK;
}

static void* ngx_http_tidehunter_create_main_conf(ngx_conf_t *cf){
    ngx_http_tidehunter_main_conf_t *mcf;
    mcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_tidehunter_main_conf_t));
    return mcf;
}

static void* ngx_http_tidehunter_create_loc_conf(ngx_conf_t *cf){
    ngx_http_tidehunter_loc_conf_t *lcf;
    lcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_tidehunter_loc_conf_t));
    return lcf;
}

static ngx_int_t ngx_http_tidehunter_rewrite_handler(ngx_http_request_t *req){
    ngx_http_tidehunter_main_conf_t *mcf = ngx_http_get_module_main_conf(req, ngx_http_tidehunter_module);
    if(mcf == NULL){
        return (NGX_DECLINED);
    }
    if(req->internal == 1){
        /* never filter on internal request */
        return (NGX_DECLINED);
    }
    ngx_array_t *filter_rule_a = mcf->head_filter_rule_a;
    ngx_http_tidehunter_filter_rule_t *filter_rule = filter_rule_a->elts;
    ngx_uint_t i;
    ngx_int_t filter_rv = 0;
    for(i=0; i < filter_rule_a->nelts; i++){
        filter_rv += filter_rule[i].filter(req, &filter_rule[i].opt);
    }
    if(filter_rv > 0){
        PRINT_INT("MATCH HIT:", (int)filter_rv);
        ngx_http_discard_request_body(req);
        return (NGX_HTTP_BAD_REQUEST);
    }
    return (NGX_DECLINED);        /* goto next handler in REWRITE PHASE */
}

static ngx_int_t ngx_http_tidehunter_rewrite_handler_body(ngx_http_request_t *req){
    if (req->method != NGX_HTTP_POST){
        ngx_http_discard_request_body(req);
        return (NGX_DECLINED);
    }
    ngx_http_tidehunter_ctx_t *ctx = ngx_http_get_module_ctx(req, ngx_http_tidehunter_module);
    if (ctx == NULL) {
        /* this handler hasn't been called */
        ctx = ngx_pcalloc(req->pool, sizeof(ngx_http_tidehunter_ctx_t));
        if (ctx == NULL) {
            LOG_ERR("ctx alloc fail", req->connection->log);
            return (NGX_ERROR);
        }
        ctx->body_done = 0;
        ngx_http_set_ctx(req, ctx, ngx_http_tidehunter_module);
        ngx_int_t rc = ngx_http_read_client_request_body(req, ngx_http_tidehunter_req_body_cb);
        if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
            return rc;
        }
        return (NGX_AGAIN);
    } else if (ctx->body_done == 0){
        /* request body cb havn't been called */
        return (NGX_AGAIN);
    } else if (ctx->match_hit > 0) {
        PRINT_INT("MATCH HIT:", (int)ctx->match_hit);
        return (NGX_HTTP_BAD_REQUEST);
    }
    return (NGX_DECLINED);
}

static void ngx_http_tidehunter_req_body_cb(ngx_http_request_t *req){
    ngx_http_tidehunter_ctx_t *ctx = ngx_http_get_module_ctx(req, ngx_http_tidehunter_module);
    if (ctx == NULL) {
        LOG_ERR("req_body_cb ctx nil", req->connection->log);
        return;
    }
    ngx_http_tidehunter_main_conf_t *mcf = ngx_http_get_module_main_conf(req, ngx_http_tidehunter_module);
    ngx_array_t *filter_rule_a = mcf->body_filter_rule_a;
    ngx_http_tidehunter_filter_rule_t *filter_rule = filter_rule_a->elts;
    ctx->body_done = 1;
    ctx->match_hit = 0;
    /* FIXME: TODO: we should not loop outside filter but inside filter, more efficient */
    ngx_uint_t i;
    for(i=0; i < filter_rule_a->nelts; i++){
        ctx->match_hit += filter_rule[i].filter(req, &filter_rule[i].opt);
    }
}

static char* ngx_http_tidehunter_test(ngx_conf_t *cf, ngx_command_t *cmd, void *conf){
    return (NGX_CONF_OK);
}
