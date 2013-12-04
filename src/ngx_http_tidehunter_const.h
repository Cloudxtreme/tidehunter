#pragma once

/*
  quite embarrasing, I cause a recursive include between module.h and filter.h.
  so I move the enums here.
 */

/* simulate a dict for mapping filters */
typedef enum {
    // FT_* is [F]ilter [T]ype
    FT_QSTR,
    FT_BODY,
    FT_TOTAL
} ngx_http_tidehunter_filter_type_e;

typedef enum {
    // MO_* is [M]atch [O]ption
    MO_EXACT_MATCH,
    MO_REG_MATCH,
    MO_EXACT_MATCH_IGNORE_CASE
} ngx_http_tidehunter_match_option_e;
