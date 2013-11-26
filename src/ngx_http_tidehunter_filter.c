#include "ngx_http_tidehunter_filter.h"

static int ngx_http_tidehunter_filter_match(ngx_http_tidehunter_filter_option_t *opt);

int ngx_http_tidehunter_filter_qstr(ngx_http_request_t *req,
                                    ngx_http_tidehunter_filter_option_t *opt){
    /* filter for the query string */
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

static int ngx_http_tidehunter_filter_match(ngx_str_t i_target_s,
                                            ngx_http_tidehunter_filter_option_t *opt){
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
