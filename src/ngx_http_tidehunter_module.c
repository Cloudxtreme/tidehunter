#include "ngx_http_tidehunter_module.h"
#include "ngx_http_tidehunter_parse.h"
#include "ngx_http_tidehunter_filter.h"
#include "ngx_http_tidehunter_smart.h"

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
        ngx_http_tidehunter_filter_qstr_init,
        NGX_HTTP_MAIN_CONF_OFFSET,
        0,
        NULL
    },
    {
        ngx_string("tidehunter_loadrule_body"),
        NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
        ngx_http_tidehunter_filter_body_init,
        NGX_HTTP_MAIN_CONF_OFFSET,
        0,
        NULL
    },
    {
        ngx_string("tidehunter_loadrule_uri"), /* uri rule ,unlike others, is located in loc conf */
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_tidehunter_filter_uri_init,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    {
        ngx_string("tidehunter_smart_thredshold"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_tidehunter_smart_init,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    {
        ngx_string("tidehunter_static_thredshold"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_num_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_tidehunter_loc_conf_t, static_thredshold),
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


    /*************************************************************************************/
    /* VERY IMPORTANT: the handlers in the same module are called in reversed order      */
    /* compared to the order in which handlers are registered to REWRITE_PHASE.handlers. */
    /* please refer to nginx source code `ngx_http.c -> ngx_http_init_phase_handlers'    */
    /*                                                                                   */
    /* so make sure the rewrite_handler_body<2> is registered right below the            */
    /* rewirte_handler_body_post<3> handler register.                                    */
    /*************************************************************************************/


    /* 3. set the post handler for request body */
    rewrite_handler_pt = ngx_array_push(&ccf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
    if(rewrite_handler_pt == NULL){
        return NGX_ERROR;
    }
    *rewrite_handler_pt = ngx_http_tidehunter_rewrite_handler_body_post;

    /* 2. set the handler for request body */
    rewrite_handler_pt = ngx_array_push(&ccf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
    if(rewrite_handler_pt == NULL){
        return NGX_ERROR;
    }
    *rewrite_handler_pt = ngx_http_tidehunter_rewrite_handler_body;

    /* 1. set the handler for request head */
    rewrite_handler_pt = ngx_array_push(&ccf->phases[NGX_HTTP_REWRITE_PHASE].handlers); /* why REWIRTE PHASE? */
    if(rewrite_handler_pt == NULL){
        return NGX_ERROR;
    }
    *rewrite_handler_pt = ngx_http_tidehunter_rewrite_handler;

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
    if (lcf == NULL) {
        return NULL;
    }
    lcf->static_thredshold = NGX_CONF_UNSET;
    return lcf;
}

static ngx_int_t ngx_http_tidehunter_rewrite_handler(ngx_http_request_t *req){
    ngx_http_tidehunter_main_conf_t *mcf = ngx_http_get_module_main_conf(req, ngx_http_tidehunter_module);
    ngx_http_tidehunter_loc_conf_t *lcf = ngx_http_get_module_loc_conf(req, ngx_http_tidehunter_module);
    if(mcf == NULL || lcf == NULL){
        return (NGX_DECLINED);
    }
    if(req->internal == 1){
        /* never filter on internal request */
        return (NGX_DECLINED);
    }
    ngx_int_t weight=0;
    /* uri filter start */
    ngx_array_t *filter_rule_a = lcf->filter_rule_a[FT_URI];
    if (filter_rule_a != NULL) {
        /*
          if the req doesn't wanna be filtered, then the weight is negative.
          and if weight>0, means the url is dangerous and you try to increase the
          chance that req get blocked.
        */
        PRINT_INFO("enter uri filter");
        weight = ngx_http_tidehunter_filter_uri(req, filter_rule_a);
        if (weight < 0) {
            req->phase_handler += 2;
            return (NGX_DECLINED);
        }
    }

    /* query string filter start */
    filter_rule_a = mcf->filter_rule_a[FT_QSTR];
    if (filter_rule_a != NULL) {
        PRINT_INFO("enter qstr filter");
        weight += ngx_http_tidehunter_filter_qstr(req, filter_rule_a);
    }
    if(weight >= 0){
        PRINT_INT("MATCH WEIGHT:", weight);
        ngx_int_t smart_rv;
        smart_rv = ngx_http_tidehunter_smart_test(req, weight);
        if (smart_rv == SMART_ABNORMAL){
            /* whether smart is NOTINIT or return ABNORMAL, deny req */
            ngx_http_discard_request_body(req);
            return (NGX_HTTP_BAD_REQUEST);
        } else if (smart_rv == SMART_NOTINIT) {
            if (lcf->static_thredshold != NGX_CONF_UNSET && weight > lcf->static_thredshold) {
                /* NOT smart init, use static thredshold. deny req */
                return (NGX_HTTP_BAD_REQUEST);
            }
        }
    }
    return (NGX_DECLINED);        /* goto next handler in REWRITE PHASE */
}

static ngx_int_t ngx_http_tidehunter_rewrite_handler_body(ngx_http_request_t *req){
    /*
      IMPLEMENTATION NOTE: to make thing easy(or worse :)).
      the callback will continue the next phase handler(tidehunter_rewrite_handler_body_post),
      when req body is ready. that is, the callback will skip this handler.

      by doing so, I can avoid using request ctx passing along to maintain some flags.
    */
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
    return (NGX_DONE);
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
    ngx_http_tidehunter_loc_conf_t *lcf = ngx_http_get_module_loc_conf(req, ngx_http_tidehunter_module);
    if(mcf == NULL || lcf == NULL){
        return (NGX_DECLINED);
    }
    ngx_int_t weight=0;
    ngx_array_t *filter_rule_a = mcf->filter_rule_a[FT_BODY];
    if (filter_rule_a != NULL) {
        ngx_http_request_body_t *rb = req->request_body;
        if (rb == NULL) {
            /* whether req body is in buf, chain or tempfile, buf won't be NULL */
            PRINT_INFO("body length zero");
        } else {
            PRINT_INFO("start body filter");
            weight = ngx_http_tidehunter_filter_body(req, filter_rule_a);
        }
    }
    if(weight >= 0){
        PRINT_INT("MATCH WEIGHT:", weight);
        ngx_int_t smart_rv;
        smart_rv = ngx_http_tidehunter_smart_test(req, weight);
        if (smart_rv == SMART_ABNORMAL){
            /* whether smart is NOTINIT or return ABNORMAL, deny req */
            ngx_http_discard_request_body(req);
            return (NGX_HTTP_BAD_REQUEST);
        } else if (smart_rv == SMART_NOTINIT) {
            if (lcf->static_thredshold != NGX_CONF_UNSET && weight > lcf->static_thredshold) {
                /* NOT smart init, use static thredshold. deny req */
                return (NGX_HTTP_BAD_REQUEST);
            }
        }
    }
    return (NGX_DECLINED);
}

static void ngx_http_tidehunter_req_body_callback(ngx_http_request_t *req){
    req->phase_handler++;       /* set phase_handler to tidehunter_rewrite_handler_body_post */
#if defined(nginx_version) && nginx_version >= 8011
    /* read_client_request_body always increments the counter */
    req->main->count--;
#endif
    ngx_http_core_run_phases(req);
}
