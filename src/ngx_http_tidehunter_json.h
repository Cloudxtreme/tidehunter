#pragma once

#include <jansson.h>

#include "ngx_http_tidehunter_common.h"
#include "ngx_http_tidehunter_filter.h"


int ngx_http_tidehunter_load_rule(ngx_str_t *fname,
                                  ngx_array_t **rule_a,
                                  ngx_pool_t *pool);
