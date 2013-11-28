#include "ngx_http_tidehunter_filter.h"
#include "ngx_http_tidehunter_json.h"

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
    if(i_target_s->len == 0){
        /* empty string */
        return rv;
    }
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
        int capture[6];
        ngx_regex_exec(opt->compile_regex->regex, i_target_s, capture, 6);
        fprintf(stderr, "capture: %d\n", capture[0]);
#else
        fprintf(stderr, "reg match rule requires PCRE library\n");
#endif
    }
    return rv;
}

int ngx_http_tidehunter_filter_init_rule(ngx_http_tidehunter_main_conf_t *mcf,
                                         ngx_pool_t *pool){
    /* hand-make a filter rule up, qstr filter */
    ngx_http_tidehunter_filter_rule_t *filter_rule = ngx_array_push(mcf->filter_rule_a);
    ngx_str_t fname = ngx_string("/tmp/test.json");
    ngx_http_tidehunter_load_rule(&fname, filter_rule, pool);
    ngx_http_tidehunter_filter_rule_t tmp_rule  = {
        ngx_string("qstr filter"), /* msg */
        ngx_string("1001"),        /* id */
        8,                         /* weight */
        ngx_http_tidehunter_filter_qstr,
        {
            MO_REG_MATCH,
            ngx_null_string,
            NULL                   /* the compile_regex ptr to be init */
        }
    };
    if(tmp_rule.opt.match_opt == MO_REG_MATCH){
        ngx_regex_compile_t *regex = ngx_pcalloc(pool, sizeof(ngx_regex_compile_t));
        ngx_str_set(&regex->pattern, "^hello$");
        regex->options = NGX_REGEX_CASELESS;
        regex->pool = pool;
        if(ngx_regex_compile(regex) != NGX_OK){
            fprintf(stderr, "regex compile fail\n");
        }
        tmp_rule.opt.compile_regex = regex;
    }
    ngx_memcpy(filter_rule, &tmp_rule, sizeof(ngx_http_tidehunter_filter_rule_t));
    /* hand-make end */
    return 0;
}
