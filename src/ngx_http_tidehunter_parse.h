#pragma once

#include "ngx_http_tidehunter_common.h"

typedef struct{
    ngx_str_t name;
    ngx_str_t value;
} qstr_dict_t;

void ngx_http_tidehunter_parse_qstr(ngx_str_t *i_qstr_s, ngx_array_t *o_qstr_dict_a);
