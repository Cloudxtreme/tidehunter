#include "ngx_http_tidehunter_json.h"

//FIXME: convert long to int
#define JSON_GET_INT(json, key) ((int)json_integer_value(json_object_get(json, key)))
#define JSON_GET_STR(json, key) json_string_value( json_object_get(json, key) )

#define JSON_DBG_INT(json, key) fprintf(stderr, key ":%d\n", JSON_GET_INT(json, key))
#define JSON_DBG_STR(json, key) fprintf(stderr, key ":%s\n", JSON_GET_STR(json, key))


static int fill_filter_rule(json_t* rule_json_obj,
                            ngx_http_tidehunter_filter_rule_t *rule,
                            ngx_pool_t *pool);


int ngx_http_tidehunter_load_rule(ngx_str_t *fname,
                                  ngx_http_tidehunter_filter_rule_t *rule,
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
    fprintf(stderr, "fname:%.*s\n", (int)fname->len, fname->data);
    json_t *json;
    json_error_t error;
    char * fname_str;
    fname_str = malloc(fname->len * sizeof(char) + 1);
    memcpy(fname_str, fname->data, fname->len);
    fname_str[fname->len] = '\0';
    json = json_load_file(fname_str, 0, &error);
    if( !json ){
        fprintf(stderr, "fail to load json file:%s.\n", fname_str);
        return -1;
    }
    free(fname_str);

    if( !json_is_array(json) ){
        fprintf(stderr, "json file hasn't a json array at top\n");
        return -2;
    }
    json_t *rule_json_obj = json_array_get(json, 0);
    if( !rule_json_obj ){
        fprintf(stderr, "fail to get rule in json array\n");
        return -3;
    }
    fill_filter_rule(rule_json_obj, rule, pool);
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
    json_t *opt_json_obj = json_object_get(rule_json_obj, "optx");
#define RS_DEBUG
#ifdef RS_DEBUG
    JSON_DBG_STR(rule_json_obj, "msg");
    JSON_DBG_STR(rule_json_obj, "id");
    JSON_DBG_INT(rule_json_obj, "weight");
    JSON_DBG_INT(rule_json_obj, "filter");
    if( json_is_object(opt_json_obj) ){
        JSON_DBG_INT(opt_json_obj, "match_opt");
        //json_array_get(opt_json_obj, "exact_str");
        //json_array_get(opt_json_obj, "regex_str");
        }
#endif
    return 0;
}
