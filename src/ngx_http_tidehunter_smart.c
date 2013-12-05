#include <math.h>

#include "ngx_http_tidehunter_smart.h"
#include "ngx_http_tidehunter_module.h"

#include "ngx_http_tidehunter_debug.h"

/*****************************************************************************************/
/* not smart at all, be careful                                                          */
/*                                                                                       */
/* IMPLEMENTATION:                                                                       */
/* this file implement a `smart filter decision maker' to auto adjust the thredshold     */
/* of which I use to determine whether a request is a BAD request.                       */
/* so I use the basic statistic such as average and standard error things.               */
/*                                                                                       */
/* to calculate the stderr(or stdvar), I should maintain a match_hit-weight history array*/
/* of size: N. the average is calculated using SMA method (Simple Moving Average).       */
/*                                                                                       */
/* and as to the stdvar, it's mean to be fast, so precise is NOT my first consideration. */
/* so the `get_stdvar' comes.                                                            */
/*****************************************************************************************/


static float get_stdvar(ngx_http_tidehunter_smart_t *smart, ngx_int_t weight);

ngx_int_t ngx_http_tidehunter_smart_test(ngx_http_request_t *req, ngx_int_t weight){
    extern ngx_module_t ngx_http_tidehunter_module;
    ngx_http_tidehunter_loc_conf_t *lcf = ngx_http_get_module_loc_conf(req, ngx_http_tidehunter_module);
    float stdvar;
    float thredshold;
    if (lcf->smart == NULL) {
        /* not init smart */
        return -2;
    }
    stdvar = get_stdvar(lcf->smart, weight);
    thredshold = stdvar + lcf->smart->average;
    PRINT_INT("thredshold:", (int)thredshold);
    if ((ngx_int_t)thredshold > weight) {
        /* it's a normal request */
        return 0;
    } else {
        /* abnormal request */
        PRINT_INFO("abnormal request");
        return -1;
    }
}

char *ngx_http_tidehunter_smart_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf){
    ngx_str_t *value = cf->args->elts; /* the initial threadshold */
    ngx_int_t thredshold = ngx_atoi(value[1].data, value[1].len);
    PRINT_INT("init thredshold:", (int)thredshold);
    ngx_http_tidehunter_loc_conf_t *lcf = conf;
    lcf->smart = ngx_pcalloc(cf->pool, sizeof(ngx_http_tidehunter_smart_t));
    if (lcf->smart == NULL) {
        return NGX_CONF_ERROR;
    }
    ngx_int_t i;
    for(i=0; i < RING_SIZE; i++){
        lcf->smart->hist_weight[i] = thredshold;
    }
    lcf->smart->average = thredshold;
    lcf->smart->stdvar = 0;
    lcf->smart->tail_pos = 0;
    lcf->smart->tail_weight = thredshold;
    return NGX_OK;
}

static float get_stdvar(ngx_http_tidehunter_smart_t *smart, ngx_int_t weight){
    /*
      every time new request is filter, the weight is calculated and passed here.
      at here, the stdvar will be recalculate based on the old `N-1' size of hist_weight
      and a new weight. that is how SMA works.
      a tail_weight in hist_weight is removed, the new weight is put in there,
      and tail_pos move one step forward.
    */
    float stdvar=smart->stdvar;
    float aver=smart->average;
    aver = aver - (smart->tail_weight / RING_SIZE) + (weight / RING_SIZE);               /* this is SMA */
    stdvar = stdvar + (pow((weight - aver), 2) -
                       pow((smart->tail_weight - smart->average), 2)) / RING_SIZE;  /* recalculate stdvar roughly, not precise */
    smart->stdvar = stdvar;
    smart->average = aver;
    smart->hist_weight[smart->tail_pos] = weight;
    smart->tail_pos = (smart->tail_pos + 1) % RING_SIZE;                                 /* this is how ring ptr move */
    smart->tail_weight = smart->hist_weight[smart->tail_pos];
    return stdvar;
}
