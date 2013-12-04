#pragma once

#include <jansson.h>

#include "ngx_http_tidehunter_common.h"
#include "ngx_http_tidehunter_filter.h"


int ngx_http_tidehunter_load_rule(ngx_http_tidehunter_main_conf_t *mcf,
                                  ngx_http_tidehunter_filter_type_e filter_type,
                                  ngx_pool_t *pool);
