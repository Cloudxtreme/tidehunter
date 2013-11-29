#include "ngx_http_tidehunter_json.h"
#include "ngx_http_tidehunter_filter.h"

//#define RS_DEBUG

#include "ngx_http_tidehunter_debug.h"

//FIXME: convert long to int
#define JSON_GET_INT(json, key) ((int)json_integer_value(json_object_get(json, key)))
#define JSON_GET_STR(json, key) json_string_value( json_object_get(json, key) )

#define JSON_DBG_INT(json, key) fprintf(stderr, key ":%d\n", JSON_GET_INT(json, key))
#define JSON_DBG_STR(json, key) fprintf(stderr, key ":%s\n", JSON_GET_STR(json, key))

static int fill_filter_rule(json_t* rule_json_obj,
                            ngx_http_tidehunter_filter_rule_t *rule,
                            ngx_pool_t *pool);

static int jsonstr2ngxstr(json_t *json, ngx_str_t *ngxstr, ngx_pool_t *pool);


int ngx_http_tidehunter_load_rule(ngx_str_t *fname,
                                  ngx_array_t *rule_a,
                                  ngx_pool_t *pool){
    /*
      json format:
      [ {
          "msg": "xxx",             string
          "id" : "xxx",             string
          "weight": xxx,            int
          "filter": filter type,    int
          "opt": {
              "match_opt":  xxx,    int
              "exact_str": "xxx",   string
              "regex_str": "xxx",   string
          }
        }, .. ]        object
    */
    PRINT_NGXSTR_PTR("fname:", fname);
    json_t *json;
    json_error_t error;
    char * fname_str;
    fname_str = malloc(fname->len * sizeof(char) + 1);
    memcpy(fname_str, fname->data, fname->len);
    fname_str[fname->len] = '\0';
    json = json_load_file(fname_str, 0, &error);
    if( !json ){
        MESSAGE2("fail to load json file:", fname_str);
        return -1;
    }
    free(fname_str);

    if( !json_is_array(json) ){
        MESSAGE("json file hasn't a json array at top");
        return -2;
    }

    size_t array_size = json_array_size(json);
    size_t i;
    for (i=0; i < array_size; i++){
        json_t *rule_json_obj = json_array_get(json, i);
        if( !rule_json_obj ){
            MESSAGE("fail to get rule in json array");
            return -3;
        }
        ngx_http_tidehunter_filter_rule_t *rule  = ngx_array_push(rule_a);
        fill_filter_rule(rule_json_obj, rule, pool);
    }

    return 0;
}

static int fill_filter_rule(json_t* rule_json_obj,
                            ngx_http_tidehunter_filter_rule_t *rule,
                            ngx_pool_t *pool){
    /*
    json_t *msg_json_str = json_object_get(rule_json_obj, "msg");
    json_t *id_json_str = json_object_get(rule_json_obj, "id");
    json_t *weight_json_int = json_object_get(rule_json_obj, "weight");
    json_t *filter_json_int = json_object_get(rule_json_obj, "filter");
    */

    jsonstr2ngxstr(json_object_get(rule_json_obj, "msg"), &rule->msg, pool);
    jsonstr2ngxstr(json_object_get(rule_json_obj, "id"), &rule->id, pool);
    rule->weight = JSON_GET_INT(rule_json_obj, "weight");
    rule->filter = ngx_http_tidehunter_filter_qstr; /* FIXME */
    json_t *opt_json_obj = json_object_get(rule_json_obj, "opt");

#ifdef RS_DEBUG
    JSON_DBG_STR(rule_json_obj, "msg");
    JSON_DBG_STR(rule_json_obj, "id");
    JSON_DBG_INT(rule_json_obj, "weight");
    JSON_DBG_INT(rule_json_obj, "filter");
    if (json_is_object(opt_json_obj)) {
        JSON_DBG_INT(opt_json_obj, "match_opt");
        JSON_DBG_STR(opt_json_obj, "exact_str");
        JSON_DBG_STR(opt_json_obj, "regex_str");
    }
#endif

    if (json_is_object(opt_json_obj)) {
        rule->opt.match_opt = JSON_GET_INT(opt_json_obj, "match_opt");
        if(rule->opt.match_opt == MO_REG_MATCH){
            ngx_regex_compile_t *rc = ngx_pcalloc(pool, sizeof(ngx_regex_compile_t));
            if (jsonstr2ngxstr(json_object_get(opt_json_obj, "regex_str"), &rc->pattern, pool) != 0){
                MESSAGE("NOT regex_str found");
                return -1;
            }
            rc->options = NGX_REGEX_CASELESS;
            rc->pool = pool;
            if(ngx_regex_compile(rc) != NGX_OK){
                MESSAGE("regex compile fail");
                return -2;
            }
            rule->opt.compile_regex = rc;
        } else {
            /* MO_EXACT_MATCH || MO_EXACT_MATCH_IGNORE_CASE */
            if (jsonstr2ngxstr(json_object_get(opt_json_obj, "exact_str"), &rule->opt.exact_str, pool) != 0){
                MESSAGE("NOT exact_str found\n");
                rule->opt.exact_str.len = 0; /* make sure exact_str is zero length */
                return -1;
            }
        }
    }

    return 0;
}

static int jsonstr2ngxstr(json_t *json, ngx_str_t *ngxstr, ngx_pool_t *pool){
    /* copy json string to ngxstr struct, alloc new memory in pool */
    const char *str;
    if(json == NULL){
        return -1;
    }
    str = json_string_value(json);
    if (str == NULL) {
        return -2;
    }
    ngxstr->len = ngx_strlen(str);
    ngxstr->data = ngx_pcalloc(pool, sizeof(char)*ngxstr->len);
    ngx_memcpy(ngxstr->data, str, ngxstr->len);
    return 0;
}
