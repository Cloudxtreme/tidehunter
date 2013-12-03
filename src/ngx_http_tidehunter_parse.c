/* functions for parsing query string. including,
   - split query string to key/value pair
   - unescape query string */

#include "ngx_http_tidehunter_parse.h"
#include "ngx_http_tidehunter_debug.h"

static int split_by_eq(ngx_str_t *i_qstr_s, ngx_uint_t start, ngx_uint_t end, qstr_dict_t *o_dict);


void ngx_http_tidehunter_parse_qstr(ngx_http_request_t *req, ngx_str_t *i_qstr_s, ngx_array_t *o_qstr_dict_a){
    /*
      parse the query string
      @param in:  i_qstr_s: query string `a=1&b=2'
      @param out: o_qstr_dict_a: tokenize query string name/value pairs

      `ab=123&bc=245'
       i    j
              i    j
              ...
    */
    ngx_uint_t i=0, j=0;
    qstr_dict_t *dict;
    for (j=0; j < i_qstr_s->len; j++) {
        if (i_qstr_s->data[j] == '&') {
            dict = (qstr_dict_t*) ngx_array_push(o_qstr_dict_a);
            if (dict == NULL) {
                LOG_ERR("array push fail", req->connection->log);
                return;
            }
            split_by_eq(i_qstr_s, i, j-1, dict);
            i = j + 1;          /* skip the `&' symbol */
        }
    }
    if(i < j){                  /* the last name/value pair */
        dict = (qstr_dict_t*) ngx_array_push(o_qstr_dict_a);
        if (dict == NULL) {
            LOG_ERR("array push fail", req->connection->log);
            return;
        }
        split_by_eq(i_qstr_s, i, j-1, dict);
    }
}


static int split_by_eq(ngx_str_t *i_qstr_s, ngx_uint_t start, ngx_uint_t end, qstr_dict_t *o_dict){
    /*
      split query string by `=' symbol.
      @params in:  i_qstr_s: query string. `a=1'
      @params out: o_dict: {name="a", value="1"}
      @return: 0 == success

      note: the o_dict's member, eg. o_dict->name, its data ptr is point to
            the original request uri memory address, so change the uri will affect it.


      `abc=123'
       |  || |
       |  || ^end
       |  |^o_dict->value->data
       |  ^eq_pos
       ^start, o_dict->name->data
     */
    if(start > end){
        return -1;
    }
    ngx_uint_t eq_pos;          /* FIXED: is it unsafe to use a int as offset instead of unsigned int? */
    for (eq_pos=start;
         eq_pos <= end && i_qstr_s->data[eq_pos] != '=';
         eq_pos++);              /* find the `=' position */
    o_dict->name.len = eq_pos - start;
    o_dict->name.data = i_qstr_s->data + start;
    if (end < eq_pos) {
        /* if end < eq_pos: then not `=' found */
        o_dict->value.len = 0;
        o_dict->value.data = NULL;
    } else {
        o_dict->value.len = end - eq_pos;
        o_dict->value.data = i_qstr_s->data + eq_pos +1;
    }
    return 0;
}

void ngx_http_tidehunter_unescape_args(ngx_http_request_t *req, ngx_str_t *dst, ngx_str_t *src){
    /* unescape string will only be shorter */
    u_char *buf = ngx_pcalloc(req->pool, src->len);
    u_char *const buf_start = buf;                   /* save the start point of buf */
    u_char *const src_start = src->data;             /* save the start point of req->args */

    if (buf == NULL) {
        ngx_log_error(NGX_LOG_ERR, req->connection->log, 0,
                      "tidehunter_unescape_uri: input data not consumed completely");
        return;
    }
    ngx_unescape_uri(&buf, &src->data, src->len, 0); /* `0' is quite hackie, refer to ngx_unescape_uri */
    if (src->data != src_start + src->len) {
        LOG_ERR("unescape_uri: input data not consumed completely", req->connection->log);
    }
    dst->len = buf - buf_start;
    dst->data = buf_start;
    src->data = src_start;                           /* the ngx_unescape_uri move the dst & src ptr both, so restore it */
}
