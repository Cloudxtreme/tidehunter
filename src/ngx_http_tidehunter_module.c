#include "ngx_http_tidehunter_module.h"
#include "ngx_http_tidehunter_parse.h"
#include "ngx_http_tidehunter_filter.h"

#include "ngx_http_tidehunter_debug.h"

static ngx_int_t ngx_http_tidehunter_init(ngx_conf_t *cf);
static void* ngx_http_tidehunter_create_main_conf(ngx_conf_t *cf);
static void* ngx_http_tidehunter_create_loc_conf(ngx_conf_t *cf);
static ngx_int_t ngx_http_tidehunter_rewrite_handler(ngx_http_request_t *req);
static ngx_int_t ngx_http_tidehunter_rewrite_handler_body(ngx_http_request_t *req);
static ngx_int_t ngx_http_tidehunter_rewrite_handler_body_post(ngx_http_request_t *req);
static void ngx_http_tidehunter_req_body_callback(ngx_http_request_t *req);


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
        ngx_string("tidehunter_loadrule_qstr"),
        NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_MAIN_CONF_OFFSET,
        offsetof(ngx_http_tidehunter_main_conf_t, qstr_rulefile),
        NULL
    },
    {
        ngx_string("tidehunter_loadrule_body"),
        NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_MAIN_CONF_OFFSET,
        offsetof(ngx_http_tidehunter_main_conf_t, body_rulefile),
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

    /* initialize the qstr filter rule */
    ngx_http_tidehunter_filter_init_rule(&mcf->qstr_rulefile, &mcf->qstr_filter_rule_a, cf->pool);

    /* initialize the REWRITE handler */
    rewrite_handler_pt = ngx_array_push(&ccf->phases[NGX_HTTP_REWRITE_PHASE].handlers); /* why REWIRTE PHASE? */
    if(rewrite_handler_pt == NULL){
        return NGX_ERROR;
    }
    *rewrite_handler_pt = ngx_http_tidehunter_rewrite_handler;

    /* initialize the handler for request body */
    rewrite_handler_pt = ngx_array_push(&ccf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
    if(rewrite_handler_pt == NULL){
        return NGX_ERROR;
    }
    *rewrite_handler_pt = ngx_http_tidehunter_rewrite_handler_body;

    /* initialize the post handler for request body */
    rewrite_handler_pt = ngx_array_push(&ccf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
    if(rewrite_handler_pt == NULL){
        return NGX_ERROR;
    }
    *rewrite_handler_pt = ngx_http_tidehunter_rewrite_handler_body_post;

    PRINT_INFO("init done");
    return NGX_OK;
}

static void* ngx_http_tidehunter_create_main_conf(ngx_conf_t *cf){
    ngx_http_tidehunter_main_conf_t *mcf;
    mcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_tidehunter_main_conf_t));
    if (mcf == NULL) {
        return NULL;
    }
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
    ngx_int_t filter_rv = 0;
    ngx_array_t *filter_rule_a = mcf->qstr_filter_rule_a;
    if (filter_rule_a != NULL) {
        ngx_http_tidehunter_filter_rule_t *filter_rule = filter_rule_a->elts;
        ngx_uint_t i;
        if (req->args.len == 0) {
            PRINT_INFO("args length zero");
        } else {
            PRINT_INFO("start qstr filter");
            for(i=0; i < filter_rule_a->nelts; i++){
                filter_rv += filter_rule[i].filter(req, &filter_rule[i].opt);
            }
        }
    }
    if(filter_rv > 0){
        PRINT_INT("MATCH HIT:", (int)filter_rv);
        ngx_http_discard_request_body(req);
        return (NGX_HTTP_BAD_REQUEST);
    }
    return (NGX_DECLINED);        /* goto next handler in REWRITE PHASE */
}

static ngx_int_t ngx_http_tidehunter_rewrite_handler_body(ngx_http_request_t *req){
    if(req->internal == 1){
        /* never filter on internal request */
        return (NGX_DECLINED);
    }
    if (req->method != NGX_HTTP_POST){
        ngx_http_discard_request_body(req);
        return (NGX_DECLINED);
    }

    ngx_int_t rc = ngx_http_read_client_request_body(req, ngx_http_tidehunter_req_body_callback);
    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
        return rc;
    }
    return (NGX_DONE);      /* IMPORTANT: to make thing easy(or worse :)).
                               the callback will continue the phase handler(tidehunter_rewrite_handler_body_post),
                               when req body is ready */
}

static ngx_int_t ngx_http_tidehunter_rewrite_handler_body_post(ngx_http_request_t *req){
    if(req->internal == 1){
        /* never filter on internal request */
        return (NGX_DECLINED);
    }
    if (req->method != NGX_HTTP_POST){
        return (NGX_DECLINED);
    }

    ngx_http_tidehunter_main_conf_t *mcf = ngx_http_get_module_main_conf(req, ngx_http_tidehunter_module);
    if(mcf == NULL){
        return (NGX_DECLINED);
    }
    ngx_int_t filter_rv = 0;
    ngx_array_t *filter_rule_a = mcf->body_filter_rule_a;
    if (filter_rule_a != NULL) {
        ngx_http_tidehunter_filter_rule_t *filter_rule = filter_rule_a->elts;
        ngx_uint_t i;
        if (req->request_body->buf == NULL) {
            /* whether req body is in buf, chain or tempfile, buf won't be NULL */
            PRINT_INFO("body length zero");
        } else {
            PRINT_INFO("start body filter");
            for(i=0; i < filter_rule_a->nelts; i++){
                filter_rv += filter_rule[i].filter(req, &filter_rule[i].opt);
            }
        }
    }
    if(filter_rv > 0){
        PRINT_INT("MATCH HIT:", (int)filter_rv);
        ngx_http_discard_request_body(req);
        return (NGX_HTTP_BAD_REQUEST);
    }

    return (NGX_DECLINED);
}

static void ngx_http_tidehunter_req_body_callback(ngx_http_request_t *req){
    req->phase_handler++;       /* set phase_handler to tidehunter_rewrite_handler_body_post */
    ngx_http_core_run_phases(req);
}
