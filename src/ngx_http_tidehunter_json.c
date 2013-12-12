#include "ngx_http_tidehunter_json.h"
#include "ngx_http_tidehunter_filter.h"
#include "ngx_http_tidehunter_const.h"

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

static int jsonstr2ngxstr(json_t *json,
                          ngx_str_t *ngxstr,
                          ngx_pool_t *pool);

static ngx_int_t load_rule(ngx_str_t rulefile,
                           ngx_array_t **filter_rule,
                           ngx_pool_t *pool);

ngx_int_t ngx_http_tidehunter_load_rule_to_mcf(ngx_http_tidehunter_main_conf_t *mcf,
                                               ngx_http_tidehunter_filter_type_e filter_type,
                                               ngx_pool_t *pool){
    /* load rule to main conf */
    return load_rule(mcf->rulefile[filter_type], &mcf->filter_rule_a[filter_type], pool);
}

ngx_int_t ngx_http_tidehunter_load_rule_to_lcf(ngx_http_tidehunter_loc_conf_t *lcf,
                                               ngx_http_tidehunter_filter_type_e filter_type,
                                               ngx_pool_t *pool){
    /* load rule to loc conf */
    return load_rule(lcf->rulefile[filter_type], &lcf->filter_rule_a[filter_type], pool);
}

static ngx_int_t load_rule(ngx_str_t rulefile,
                           ngx_array_t **filter_rule,
                           ngx_pool_t *pool){
    /*
      @return: 0 == success

      json format:
      [ {
          "msg": "xxx",         string
          "id" : "xxx",         string
          "weight": xxx,        int
          "match_opt":  xxx,    int
          "exact_str": "xxx",   string
          "regex_str": "xxx",   string
        }, .. ]        object
    */
    PRINT_NGXSTR("rule name:", rulefile);
    json_t *json;
    json_error_t error;
    char * fname_str;
    fname_str = malloc(rulefile.len * sizeof(char) + 1);
    memcpy(fname_str, rulefile.data, rulefile.len);
    fname_str[rulefile.len] = '\0';
    json = json_load_file(fname_str, 0, &error);
    if( !json ){
        MESSAGE_S("fail to load json file:", fname_str);
        return -1;
    }
    free(fname_str);

    if( !json_is_array(json) ){
        MESSAGE("json file hasn't a json array at top");
        return -2;
    }

    size_t array_size = json_array_size(json);
    size_t i;
    ngx_array_t *rule_a;
    rule_a = ngx_array_create(pool, array_size, sizeof(ngx_http_tidehunter_filter_rule_t));
    for (i=0; i < array_size; i++){
        json_t *rule_json_obj = json_array_get(json, i);
        if( !rule_json_obj ){
            MESSAGE("fail to get rule in json array");
            return -3;
        }
        ngx_http_tidehunter_filter_rule_t *rule  = ngx_array_push(rule_a);
        fill_filter_rule(rule_json_obj, rule, pool);
    }
    *filter_rule = rule_a;
    PRINT_INFO("rule loaded");
    return 0;
}

static int fill_filter_rule(json_t* rule_json_obj,
                            ngx_http_tidehunter_filter_rule_t *rule,
                            ngx_pool_t *pool){
    /*
      @return: 0 == success
      fill the filter_rule_t with json data
    */

#ifdef RS_DEBUG
    JSON_DBG_STR(rule_json_obj, "msg");
    JSON_DBG_STR(rule_json_obj, "id");
    JSON_DBG_INT(rule_json_obj, "weight");
    JSON_DBG_INT(rule_json_obj, "match_opt");
    JSON_DBG_STR(rule_json_obj, "exact_str");
    JSON_DBG_STR(rule_json_obj, "regex_str");
#endif

    u_char errstr[NGX_MAX_CONF_ERRSTR];

    jsonstr2ngxstr(json_object_get(rule_json_obj, "msg"), &rule->msg, pool);
    jsonstr2ngxstr(json_object_get(rule_json_obj, "id"), &rule->id, pool);
    rule->weight = JSON_GET_INT(rule_json_obj, "weight");

    rule->match_opt = JSON_GET_INT(rule_json_obj, "match_opt");
    if(rule->match_opt == MO_REG_MATCH){
        ngx_regex_compile_t *rc = ngx_pcalloc(pool, sizeof(ngx_regex_compile_t));
        if (jsonstr2ngxstr(json_object_get(rule_json_obj, "regex_str"), &rc->pattern, pool) != 0){
            MESSAGE("NOT regex_str found");
            JSON_DBG_STR(rule_json_obj, "id");
            return -1;
        }
        rc->options = NGX_REGEX_CASELESS; /* TODO:make it configurable? */
        rc->pool = pool;
        rc->err.len = NGX_MAX_CONF_ERRSTR;
        rc->err.data = errstr; /* the errstr is on the stack */
        if(ngx_regex_compile(rc) != NGX_OK){
            MESSAGE("regex compile fail");
            JSON_DBG_STR(rule_json_obj, "id");
            PRINT_NGXSTR("err", rc->err);
            return -2;
        }
        rule->compile_regex = rc;
    } else {
        /* MO_EXACT_MATCH || MO_EXACT_MATCH_IGNORE_CASE */
        if (jsonstr2ngxstr(json_object_get(rule_json_obj, "exact_str"), &rule->exact_str, pool) != 0){
            MESSAGE("NOT exact_str found\n");
            rule->exact_str.len = 0; /* make sure exact_str is zero length */
            return -3;
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
    ngxstr->data = ngx_pcalloc(pool, sizeof(u_char)*(ngxstr->len+1)); /* save one more byte for `\0' */
    ngx_memcpy(ngxstr->data, str, ngxstr->len);
    return 0;
}
