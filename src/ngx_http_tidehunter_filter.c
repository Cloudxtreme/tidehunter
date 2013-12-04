#include "ngx_http_tidehunter_common.h"
#include "ngx_http_tidehunter_filter.h"
#include "ngx_http_tidehunter_parse.h"
#include "ngx_http_tidehunter_json.h"

#include "ngx_http_tidehunter_debug.h"


filter_func_ptr_t filter_funcs[FT_TOTAL]; /* the one and only global */

static int ngx_http_tidehunter_filter_match(ngx_str_t *i_target_s,
                                            ngx_http_tidehunter_filter_option_t *opt);

ngx_int_t ngx_http_tidehunter_filter_qstr(ngx_http_request_t *req,
                                          ngx_http_tidehunter_filter_option_t *opt){
    /*
      filter for the query string
      @param in: req: http request
      @param in: opt: filter option
      @return: match count
    */
    ngx_array_t *qstr_dict_a = ngx_array_create(req->pool, 2, sizeof(qstr_dict_t));
    if (qstr_dict_a == NULL) {
        LOG_ERR("tidehunter_filter_qstr array init failed", req->connection->log);
        return 0;
    }
    ngx_str_t unescape_args;
    ngx_http_tidehunter_unescape_args(req, &unescape_args, &req->args);
    ngx_http_tidehunter_parse_qstr(req, &unescape_args, qstr_dict_a); /* FIXME: should NOT parse for every rule */
    qstr_dict_t *qstr_dict = qstr_dict_a->elts;
    int match_hit = 0;
    ngx_uint_t i;
    int filter_rv;
    for(i=0; i < qstr_dict_a->nelts; i++){
        filter_rv = ngx_http_tidehunter_filter_match(&qstr_dict[i].name, opt);
        if (filter_rv == 0) match_hit++;
        filter_rv = ngx_http_tidehunter_filter_match(&qstr_dict[i].value, opt);
        if (filter_rv == 0) match_hit++;
    }
    return match_hit;
}

/*
ngx_int_t ngx_http_tidehunter_filter_body(ngx_http_request_t *req,
                                    ngx_http_tidehunter_filter_option_t *opt){
    // test whether rb is in BUFS, BUF, TEMP_FILE
    if (!rb->temp_file) {
        // request body is in the buf OR bufs chain
        if (rb->bufs->next == NULL) {
            // request body in buf
        }
    } else {
        // request body is in a temp file.
    }
}
*/

static int ngx_http_tidehunter_filter_match(ngx_str_t *i_target_s,
                                            ngx_http_tidehunter_filter_option_t *opt){
    /*
      @param in: i_target_s: ngx_str_t to be match
      @param in: opt       : contains match pattern
      @return: `0' == match. `not 0' == NOT match
    */
    if (i_target_s->len == 0) {
        /* empty string */
        return -1;
    }
    if (opt->exact_str.len > i_target_s->len) {
        /* target string is shorter than match str, quit with NOT match */
        return -1;
    }
    if (opt->match_opt == MO_EXACT_MATCH) {
        if (opt->exact_str.len == 0) {
            return -1;
        }
        return ngx_strncmp(i_target_s->data, opt->exact_str.data, opt->exact_str.len); /* FIXME: this return sucks */
    } else if (opt->match_opt == MO_EXACT_MATCH_IGNORE_CASE) {
        if (opt->exact_str.len == 0) {
            return -1;
        }
        return ngx_strncasecmp(i_target_s->data, opt->exact_str.data, opt->exact_str.len);
    } else if (opt->match_opt == MO_REG_MATCH){
#if (NGX_PCRE)
        int capture[3];         /* a multlple of 3, require by pcre */
        if (opt->compile_regex == NULL) {
            return -1;
        }
        ngx_regex_exec(opt->compile_regex->regex, i_target_s, capture, 3);
        if (capture[0] > -1) {
            return 0;
        }
#else
        fprintf(stderr, "reg match rule requires PCRE library\n");
#endif
    }
    return -1;                  /* no one go here */
}


ngx_int_t ngx_http_tidehunter_filter_init_rule(ngx_http_tidehunter_main_conf_t *mcf,
                                               ngx_http_tidehunter_filter_type_e filter_type,
                                               ngx_pool_t *pool){
    /* load_rule implementation is independent, json, yaml
       whatever you want. a json rule loader in currently
       implemented by me. */
    filter_funcs[FT_QSTR] = ngx_http_tidehunter_filter_qstr;
    // filter_funcs[FT_BODY] = ngx_http_tidehunter_filter_body;
    return ngx_http_tidehunter_load_rule(mcf , filter_type, pool);
}
