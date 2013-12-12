#pragma once

#include <jansson.h>

#include "ngx_http_tidehunter_common.h"
#include "ngx_http_tidehunter_filter.h"


ngx_int_t ngx_http_tidehunter_load_rule_to_mcf(ngx_http_tidehunter_main_conf_t *mcf,
                                               ngx_http_tidehunter_filter_type_e filter_type,
                                               ngx_pool_t *pool);

ngx_int_t ngx_http_tidehunter_load_rule_to_lcf(ngx_http_tidehunter_loc_conf_t *lcf,
                                               ngx_http_tidehunter_filter_type_e filter_type,
                                               ngx_pool_t *pool);
