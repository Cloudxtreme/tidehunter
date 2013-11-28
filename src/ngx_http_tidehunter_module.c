#include "ngx_http_tidehunter_module.h"
#include "ngx_http_tidehunter_parse.h"
#include "ngx_http_tidehunter_filter.h"

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
    rewrite_handler_pt = ngx_array_push(&ccf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
    if(rewrite_handler_pt == NULL){
        return NGX_ERROR;
    }
    *rewrite_handler_pt = ngx_http_tidehunter_rewrite_handler;
    return NGX_OK;
}

static void* ngx_http_tidehunter_create_main_conf(ngx_conf_t *cf){
    ngx_http_tidehunter_main_conf_t *mcf;
    mcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_tidehunter_main_conf_t));
    mcf->filter_rule_a = ngx_array_create(cf->pool, 2, sizeof(ngx_http_tidehunter_filter_rule_t));
#define DEBUG
#ifdef DEBUG
    /* hand-make a filter rule up, qstr filter */
    ngx_http_tidehunter_filter_rule_t *tmp_rule = ngx_array_push(mcf->filter_rule_a);
    ngx_http_tidehunter_filter_rule_t tmp  = {
        ngx_string("qstr filter"), /* msg */
        ngx_string("1001"),        /* id */
        8,                         /* weight */
        ngx_http_tidehunter_filter_qstr,
        {
            MO_REG_MATCH,
            ngx_string("hello"),
            NULL,                   /* the regex ptr */
        }
    };
    if(tmp.opt.match_opt == MO_REG_MATCH){
        ngx_regex_compile_t *regex = ngx_pcalloc(cf->pool, sizeof(ngx_regex_compile_t));
        ngx_str_set(&regex->pattern, "hello");
        regex->options = NGX_REGEX_CASELESS;
        regex->pool = cf->pool;
        if(ngx_regex_compile(regex) != NGX_OK){
            fprintf(stderr, "regex compile fail\n");
        }
        tmp.opt.compile_regex = regex;
    }
    ngx_memcpy(tmp_rule, &tmp, sizeof(ngx_http_tidehunter_filter_rule_t));
    /* hand-make end */
#endif
    return mcf;
}

static void* ngx_http_tidehunter_create_loc_conf(ngx_conf_t *cf){
    ngx_http_tidehunter_loc_conf_t *lcf;
    lcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_tidehunter_loc_conf_t));
    return lcf;
}

static ngx_int_t ngx_http_tidehunter_rewrite_handler(ngx_http_request_t *req){
    ngx_http_tidehunter_main_conf_t *mcf = ngx_http_get_module_main_conf(req,ngx_http_tidehunter_module);
    if(mcf == NULL){
        return (NGX_DECLINED);
    }
    if(req->internal == 1){
        return (NGX_DECLINED);
    }
    ngx_array_t *filter_rule_a = mcf->filter_rule_a;
    ngx_http_tidehunter_filter_rule_t *filter_rule = filter_rule_a->elts;
    ngx_uint_t i;
    int filter_rv;
    for(i=0; i < filter_rule_a->nelts; i++){
        // fprintf(stderr, "msg==%.*s\n", (int)filter_rule[i].msg.len, filter_rule[i].msg.data);
        filter_rv = filter_rule[i].filter(req, &filter_rule[i].opt);
        if(filter_rv > 0){
            fprintf(stderr, "MATCH HIT:%d\n", filter_rv);
            return NGX_HTTP_BAD_REQUEST;
        }
    }
    return (NGX_DECLINED);        /* goto next handler in REWRITE PHASE */
}

/*
static ngx_int_t ngx_http_tidehunter_rewrite_handler_2(ngx_http_request_t *req){
    ngx_array_t *qstr_dict_a = ngx_array_create(req->pool, 2, sizeof(qstr_dict_t));
    ngx_http_tidehunter_parse_qstr(&req->args, qstr_dict_a);
    qstr_dict_t *qstr_dict = qstr_dict_a->elts;
    ngx_uint_t i;
    for(i=0; i < qstr_dict_a->nelts; i++){
        fprintf(stderr, "[%.*s==%.*s]\n", (int)qstr_dict[i].name.len, qstr_dict[i].name.data,
                                          (int)qstr_dict[i].value.len, qstr_dict[i].value.data);
    }
    return NGX_DECLINED;
}
*/

static char* ngx_http_tidehunter_test(ngx_conf_t *cf, ngx_command_t *cmd, void *conf){
    return (NGX_CONF_OK);
}
