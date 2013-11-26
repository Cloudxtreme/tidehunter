/* functions for parsing query string. */

#include "ngx_http_tidehunter_parse.h"

static int split_by_eq(ngx_str_t *i_qstr_s, int start, int end, qstr_dict_t *o_dict);

void ngx_http_tidehunter_parse_qstr(ngx_str_t *i_qstr_s, ngx_array_t *o_qstr_dict_a){
    // parse the query string
    // @param in:  i_qstr_s: query string `a=1&b=2'
    // @param out: o_qstr_dict_a: tokenize query string name/value pairs
    /*
      `ab=123&bc=245'
       i    j
              i    j
              ...
    */
    int i=0, j=0;
    qstr_dict_t *dict;
    for(j=0; j < (int)i_qstr_s->len; j++){
        if(i_qstr_s->data[j] == '&'){
            dict = (qstr_dict_t*) ngx_array_push(o_qstr_dict_a);
            split_by_eq(i_qstr_s, i, j-1, dict);
            i = j + 1;          /* skip the `&' symbol */
        }
    }
    if(i < j){                  /* the last name/value pair */
        dict = (qstr_dict_t*) ngx_array_push(o_qstr_dict_a);
        split_by_eq(i_qstr_s, i, j-1, dict);
    }
}

static int split_by_eq(ngx_str_t *i_qstr_s, int start, int end, qstr_dict_t *o_dict){
    // split query string by `=' symbol.
    // @params in:  i_qstr_s: query string. `a=1'
    // @params out: o_dict: {name="a", value="1"}
    //
    // note: the o_dict's member, eg. o_dict->name, its data ptr is point to
    //       the original request uri memory address, so change the uri will affect it.
    /*
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
    int eq_pos;                 /* FIXME: is it unsafe to use a int as offset instead of unsigned int? */
    for(eq_pos=start;
        eq_pos <= end && i_qstr_s->data[eq_pos] != '=';
        eq_pos++);              /* find the `=' position */
    o_dict->name.len = eq_pos - start;
    o_dict->name.data = i_qstr_s->data + start;
    if(end < eq_pos){
        /* if end < eq_pos: then not `=' found */
        o_dict->value.len = 0;
        o_dict->value.data = NULL;
    } else {
        o_dict->value.len = end - eq_pos;
        o_dict->value.data = i_qstr_s->data + eq_pos +1;
    }
    return 0;
}
