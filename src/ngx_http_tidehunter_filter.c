#include "ngx_http_tidehunter_filter.h"

static int ngx_http_tidehunter_filter_match(ngx_str_t *i_target_s,
                                            ngx_http_tidehunter_filter_option_t *opt);

int ngx_http_tidehunter_filter_qstr(ngx_http_request_t *req,
                                    ngx_http_tidehunter_filter_option_t *opt){
    /*
      filter for the query string
      @param in: req: http request
      @param in: opt: filter option
      @return: match count
    */
    ngx_array_t *qstr_dict_a = ngx_array_create(req->pool, 2, sizeof(qstr_dict_t));
    ngx_http_tidehunter_parse_qstr(&req->args, qstr_dict_a);
    qstr_dict_t *qstr_dict = qstr_dict_a->elts;
    int match_hit = 0;
    ngx_uint_t i;
    int filter_rv;
    for(i=0; i < qstr_dict_a->nelts; i++){
        filter_rv = ngx_http_tidehunter_filter_match(&qstr_dict[i].name, opt);
        if(filter_rv == 0) match_hit++;
        filter_rv = ngx_http_tidehunter_filter_match(&qstr_dict[i].value, opt);
        if(filter_rv == 0) match_hit++;
    }
    return match_hit;
}

static int ngx_http_tidehunter_filter_match(ngx_str_t *i_target_s,
                                            ngx_http_tidehunter_filter_option_t *opt){
    /*
      @param in: i_target_s: ngx_str_t to be match
      @param in: opt       : contains match pattern
      @return: `0' == match. `1' == NOT match
    */
    int rv = 1;                 /* default is NOT match: (rv != 1) */
    if(opt->exact_match_s.len > i_target_s->len){
        /* target string is shorter than match str, quit with NOT match */
        return rv;
    }
    if(opt->match_opt == MO_EXACT_MATCH){
        rv = ngx_strncmp(i_target_s->data, opt->exact_match_s.data, opt->exact_match_s.len);
    } else if(opt->match_opt == MO_EXACT_MATCH_IGNORE_CASE){
        rv = ngx_strncasecmp(i_target_s->data, opt->exact_match_s.data, opt->exact_match_s.len);
    } else if(opt->match_opt == MO_REG_MATCH){
        //FIXME
#if (NGX_PCRE)
        fprintf(stderr, "not implemented yet\n");
#else
        fprintf(stderr, "reg match rule requires PCRE library\n");
#endif
    }
    return rv;
}
