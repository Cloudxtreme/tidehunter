#include "ngx_http_tidehunter_common.h"
#include "ngx_http_tidehunter_filter.h"
#include "ngx_http_tidehunter_parse.h"
#include "ngx_http_tidehunter_json.h"

#include "ngx_http_tidehunter_debug.h"


static int ngx_http_tidehunter_filter_match(ngx_str_t *i_target_s,
                                            ngx_http_tidehunter_filter_rule_t *rule);

ngx_int_t ngx_http_tidehunter_filter_qstr(ngx_http_request_t *req,
                                          ngx_array_t *rule_a){
    /*
      filter for the query string
      @param in: req: http request
      @param in: opt: filter rule array
      @return: match weight
    */
    /*
      TODO: do we need utf8 supported in query string while filtering dangerous keyword?
            currently the rules I copy from naxsi give no care about anything except ascii.
    */
    ngx_array_t *qstr_dict_a = ngx_array_create(req->pool, 2, sizeof(qstr_dict_t));
    if (qstr_dict_a == NULL) {
        LOG_ERR("tidehunter_filter_qstr array init failed", req->connection->log);
        return 0;
    }
    ngx_str_t unescape_args;
    ngx_http_tidehunter_unescape_args(req, &unescape_args, &req->args);
    ngx_http_tidehunter_parse_qstr(req, &unescape_args, qstr_dict_a);
    qstr_dict_t *qstr_dict = qstr_dict_a->elts;
    ngx_int_t weight = 0;
    ngx_uint_t i, j;
    ngx_http_tidehunter_filter_rule_t *rule_elts = rule_a->elts;
    for (i=0; i < qstr_dict_a->nelts; i++) {
        for (j=0; j < rule_a->nelts; j++) {
            if (ngx_http_tidehunter_filter_match(&qstr_dict[i].name, &rule_elts[j])) {
                weight += rule_elts[j].weight;
            }
            if (ngx_http_tidehunter_filter_match(&qstr_dict[i].value, &rule_elts[j])) {
                weight += rule_elts[j].weight;
            }
        }
    }
    return weight;
}


ngx_int_t ngx_http_tidehunter_filter_body(ngx_http_request_t *req,
                                          ngx_array_t *rule_a){
    /*
      filter for the request body
      @param in: req: http request
      @param in: opt: filter rule array
      @return: match weight
    */
    /*
      TODO: actually this is very far from good.
        - test the content-type so to determine how to decode the body
        - currently big body(in tempfile) is ignored.
    */
    ngx_http_request_body_t *rb = req->request_body;
    // test whether rb is in BUFS, BUF, TEMP_FILE
    if (req->headers_in.content_length_n <=0) {
        return 0;
    }
    if (!rb->temp_file) {
        // request body is in the buf OR bufs chain
        if (rb->bufs != NULL) {
            // request body in buf
            ngx_uint_t bufs_len=0;
            ngx_chain_t* buf_next = rb->bufs;
            do {
                bufs_len += (buf_next->buf->last - buf_next->buf->pos);
                buf_next = buf_next->next;
            } while(buf_next != NULL);
            ngx_str_t buf;
            buf.len = bufs_len;
            buf.data = (u_char*)malloc(bufs_len);
            off_t offset=0;
            buf_next = rb->bufs;
            do {
                ngx_memcpy(buf.data + offset, buf_next->buf->pos, buf_next->buf->last - buf_next->buf->pos);
                offset += buf_next->buf->last - buf_next->buf->pos;
                buf_next = buf_next->next;
            } while(buf_next != NULL);
            ngx_int_t weight=0;
            ngx_uint_t j;
            ngx_http_tidehunter_filter_rule_t *rule_elts = rule_a->elts;
            for (j=0; j < rule_a->nelts; j++) {
                if (ngx_http_tidehunter_filter_match(&buf, &rule_elts[j])) {
                    weight += rule_elts[j].weight;
                }
            }
            return weight;
        } else {
            PRINT_INFO("where is body?");
            return 0;
        }
    } else {
        // request body is in a temp file. FIXME: to deal with big file?
        PRINT_INFO("body in temp file");
        return 0;
    }
}


static int ngx_http_tidehunter_filter_match(ngx_str_t *i_target_s,
                                            ngx_http_tidehunter_filter_rule_t *rule){
    /*
      the basic function for filter.
      @param in: i_target_s: ngx_str_t to be match
      @param in: opt       : contains match pattern
      @return: `0' == NOT match. `1' == match
    */
    if (i_target_s->len == 0) {
        /* empty string */
        return 0;
    }
    if (rule->exact_str.len > i_target_s->len) {
        /* target string is shorter than match str, quit with NOT match */
        return 0;
    }
    if (rule->match_opt == MO_EXACT_MATCH) {
        if (rule->exact_str.len == 0) {
            return 0;
        }
        if (ngx_strncmp(i_target_s->data, rule->exact_str.data, rule->exact_str.len) == 0) {
            return 1;
        }
    } else if (rule->match_opt == MO_EXACT_MATCH_IGNORE_CASE) {
        if (rule->exact_str.len == 0) {
            return 0;
        }
        if (ngx_strncasecmp(i_target_s->data, rule->exact_str.data, rule->exact_str.len) == 0){
            return 1;
        }
    } else if (rule->match_opt == MO_REG_MATCH){
#if (NGX_PCRE)
        int capture[3];         /* a multlple of 3, require by pcre */
        if (rule->compile_regex == NULL) {
            return 0;
        }
        ngx_regex_exec(rule->compile_regex->regex, i_target_s, capture, 3);
        if (capture[0] > -1) {
            return 1;
        }
#else
        fprintf(stderr, "reg match rule requires PCRE library\n");
#endif
    }
    return 0;                   /* NOT match */
}



ngx_int_t ngx_http_tidehunter_filter_init_rule(ngx_http_tidehunter_main_conf_t *mcf,
                                               ngx_http_tidehunter_filter_type_e filter_type,
                                               ngx_pool_t *pool){
    /*
      @return: 0 == success

      load_rule implementation is independent, json, yaml
      whatever you want. a json rule loader is currently
      implemented by me.
    */
    if (mcf->rulefile[filter_type].len == 0){
        /* no loadrule directive for this filter_type */
        return -1;
    }
    return ngx_http_tidehunter_load_rule(mcf , filter_type, pool);
}
