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
    ngx_http_tidehunter_unescape_uri(req, &unescape_args, &req->args);
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
    if (req->headers_in.content_length_n <=0) {
        return 0;
    }
    if (!rb->temp_file) {
        /* request body is in the buf OR bufs chain */
        if (rb->bufs != NULL) {
            /* request body in buf */
            ngx_str_t orig_ctn, unescaped_ctn;
            orig_ctn.len = req->headers_in.content_length_n; /* WHY: content_length_n is of type `off_t' */
            orig_ctn.data = ngx_palloc(req->pool, orig_ctn.len); /* alloc, but not clean needed. */
            off_t offset=0;
            ngx_chain_t *buf_next;
            buf_next = rb->bufs;
            do {                /* copy all bufs content into one continent buffer */
                ngx_memcpy(orig_ctn.data + offset, buf_next->buf->pos, buf_next->buf->last - buf_next->buf->pos);
                offset += buf_next->buf->last - buf_next->buf->pos;
                buf_next = buf_next->next;
            } while(buf_next != NULL);
            if (ngx_strcasecmp(req->headers_in.content_type->value.data,
                                /*
                                  What The Fuck! the content_type is of type `ngx_table_elt_t'
                                  and the member `value' of content_type is of type `void *',
                                  then how the hell here, it figures out the `value' is a ngx_str_t !

                                  BTW: I can't even figure out how the `content_type' in ngx_http_headers_in_t
                                  is initialized... bloody fuck!
                                  all I know is `ngx_http_headers_in_t.content_type.value' is pointed to a member
                                  of ngx_list_t `ngx_http_headers_in_t.headers'. and not clue to digg further.

                                  Fuck fUck fuCk fucK!!!!! damn it nginx!
                                 */
                               (u_char*)"application/x-www-form-urlencoded") == 0){
                /* if the content-type is urlencoded, then unescape content to a copy */
                ngx_http_tidehunter_unescape_uri(req, &unescaped_ctn, &orig_ctn);
            } else {
                unescaped_ctn = orig_ctn;
            }
            ngx_int_t weight=0;
            ngx_uint_t j;
            ngx_http_tidehunter_filter_rule_t *rule_elts = rule_a->elts;
            for (j=0; j < rule_a->nelts; j++) {
                if (ngx_http_tidehunter_filter_match(&unescaped_ctn, &rule_elts[j])) {
                    weight += rule_elts[j].weight;
                }
            }
            return weight;
        } else {
            MESSAGE("where is body?");
            return 0;
        }
    } else {
        // request body is in a temp file. FIXME: to deal with big file?
        PRINT_INFO("body in temp file");
        return 0;
    }
}


ngx_int_t ngx_http_tidehunter_filter_uri(ngx_http_request_t *req,
                                         ngx_array_t *rule_a){
    /*
      a uri filter is NOT exactly a filter, it's more like a whitelist (and a blacklist :) ).
      I create this filter so to make it possible that user can setup some
      uri whitelist at location level.

      when writing rules, please NOTICE:
        - a negative weight means I don't wanna be filtered no matter how illegal
          this req is!
        - a possitive weight means I wanna increase the chance that the req get blocked.
    */
    PRINT_NGXSTR("request uri:", req->uri);
    ngx_int_t weight=0;
    ngx_uint_t j;
    ngx_http_tidehunter_filter_rule_t *rule_elts = rule_a->elts;
    for (j=0; j < rule_a->nelts; j++) {
        if (ngx_http_tidehunter_filter_match(&req->uri, &rule_elts[j])) {
            weight += rule_elts[j].weight;
            PRINT_INT("uri weight:", weight);
        }
    }
    return weight;
}


static int ngx_http_tidehunter_filter_match(ngx_str_t *i_target_s,
                                            ngx_http_tidehunter_filter_rule_t *rule){
    /*
      the basic function for filter: apply the rule to target ngxstr
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



char *ngx_http_tidehunter_filter_qstr_init(ngx_conf_t *cf,
                                           ngx_command_t *cmd,
                                           void *conf){
    /*
      @return: 0 == success

      load_rule_to_xxx implementation is independent, json, yaml
      whatever you want. a json rule loader is currently
      implemented by me.
    */
    ngx_http_tidehunter_main_conf_t *mcf = conf;
    ngx_str_t *value = cf->args->elts;
    mcf->rulefile[FT_QSTR] = value[1];
    if (ngx_http_tidehunter_load_rule_to_mcf(mcf , FT_QSTR, cf->pool) != 0) {
        return NGX_CONF_ERROR;
    }
    return NGX_OK;
}

char *ngx_http_tidehunter_filter_body_init(ngx_conf_t *cf,
                                           ngx_command_t *cmd,
                                           void *conf){
    /*
      @return: 0 == success

      load_rule_to_xxx implementation is independent, json, yaml
      whatever you want. a json rule loader is currently
      implemented by me.
    */
    ngx_http_tidehunter_main_conf_t *mcf = conf;
    ngx_str_t *value = cf->args->elts;
    mcf->rulefile[FT_BODY] = value[1];
    if (ngx_http_tidehunter_load_rule_to_mcf(mcf , FT_BODY, cf->pool) != 0) {
        return NGX_CONF_ERROR;
    }
    return NGX_OK;
}

char *ngx_http_tidehunter_filter_uri_init(ngx_conf_t *cf,
                                          ngx_command_t *cmd,
                                          void *conf){
    /*
      @return: 0 == success

      load_rule_to_xxx implementation is independent, json, yaml
      whatever you want. a json rule loader is currently
      implemented by me.
    */
    ngx_http_tidehunter_loc_conf_t *lcf = conf;
    ngx_str_t *value = cf->args->elts;
    lcf->rulefile[FT_URI] = value[1];
    if (ngx_http_tidehunter_load_rule_to_lcf(lcf , FT_URI, cf->pool) != 0) {
        return NGX_CONF_ERROR;
    }
    return NGX_OK;
}
