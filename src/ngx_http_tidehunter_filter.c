#include "ngx_http_tidehunter_common.h"
#include "ngx_http_tidehunter_parse.h"

typedef struct{
    ngx_str_t msg;
    ngx_str_t id;
    ngx_int_t weight;
    int (*filter)(ngx_http_tidehunter_filter_options_t *opt); /* a filter func ptr */
    ngx_http_tidehunter_filter_option_t opt;              /* option for filter func ptr */
} ngx_http_tidehunter_filter_rule_t;

typedef enum{
    MO_EXACT_MATCH = 0;
    MO_REG_MATCH;
    MO_EXACT_MATCH_IGNORE_CASE;
} ngx_http_tidehunter_match_option_e;

typedef struct{
    ngx_http_tidehunter_match_option_e match_opt;
    ngx_str_t exact_match_s;
    ngx_str_t reg_match_s;
} ngx_http_tidehunter_filter_option_t;

static int ngx_http_tidehunter_filter_match(ngx_http_tidehunter_filter_option_t *opt);

static int ngx_http_tidehunter_filter_qstr(ngx_http_request_t *req,
                                           ngx_http_tidehunter_filter_option_t *opt){
    /* filter for the query string */n
    ngx_http_loc_conf_t *loc_cf = ngx_get_http_loc_conf(); //FIXME
    ngx_http_main_conf_t *main_cf = ngx_get_http_main_conf(); //FIXME

    ngx_array_t *qstr_dict_a = ngx_array_push(req->pool, 2, sizeof(qstr_dict_t));
    ngx_http_tidehunter_parse_qstr(req->args, qstr_dict_a);
    qstr_dict_t *qstr_dict = qstr_dict_a->elts;
    int rv = 0, i;
    for(i=0; i < qstr_dict_a->nelts; i++){
        ngx_http_tidehunter_filter_match(qstr_dict->name, opt);
        ngx_http_tidehunter_filter_match(qstr_dict->value, opt);
    }
    return rv;
}

static int ngx_http_tidehunter_filter_body(ngx_http_tidehunter_filter_option_t *opt){
}

static int ngx_http_tidehunter_filter_match(ngx_str_t i_target_s, ngx_http_tidehunter_filter_option_t *opt){
    if(opt->match_opt == MO_EXACT_MATCH){
        ngx_strncmp(i_target_s->data, opt->exact_match_s.data, opt->exact_match_s.len);
    } else if(opt->match_opt == MO_EXACT_MATCH_IGNORE_CASE){
        ngx_strncasecmp(i_target_s->data, opt->exact_match_s.data, opt->exact_match_s.len);
    } else if(opt->match_opt == MO_REG_MATCH){
        //FIXME
        fprintf(stderr, "not implemented\n");
    }
    return 0;
}
