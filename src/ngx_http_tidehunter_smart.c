//#include <math.h>   //no need now

#include "ngx_http_tidehunter_smart.h"
#include "ngx_http_tidehunter_module.h"

#include "ngx_http_tidehunter_debug.h"

#define POW2(x) ((x) * (x))
#define ABS(x) ( (x) > 0 ? (x) : -(x) )

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
static float Q_rsqrt( float number );

ngx_int_t ngx_http_tidehunter_smart_test(ngx_http_request_t *req, ngx_int_t weight){
    /*
      @return: SMART_NORMAL / ABNORMAL / NOTINIT
      use the `smart' method to test whether this request is normal or not.
     */
    extern ngx_module_t ngx_http_tidehunter_module;
    ngx_http_tidehunter_loc_conf_t *lcf = ngx_http_get_module_loc_conf(req, ngx_http_tidehunter_module);
    float stdvar;
    float thredshold;
    if (lcf->smart == NULL) {
        /* not init smart */
        return SMART_NOTINIT;
    }
    stdvar = get_stdvar(lcf->smart, weight);
    if (stdvar <= 1e-3) {
        /* here may need some math? `stdvar <= 1/N' is better?*/
        thredshold = lcf->smart->average;
    } else {
        /* the `0.5' is for ceil-ing the thredshold then convert to ngx_int_t */
        thredshold = ABS(1/Q_rsqrt(stdvar)) + lcf->smart->average;
    }
    PRINT_FLOAT("thredshold:", thredshold);
    PRINT_FLOAT("stdvar:", stdvar);
    PRINT_INT("aver:", lcf->smart->average);
    PRINT_INT("tail weight:", lcf->smart->tail_weight);
    if (thredshold >= weight) {
        /* it's a normal request */
        return SMART_NORMAL;
    } else {
        /* abnormal request */
        PRINT_INFO("abnormal request");
        return SMART_ABNORMAL;
    }
}

char *ngx_http_tidehunter_smart_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf){
    /*
      directive command function.
      set up initial thredshold, spread this thredshold to all the `smart ring' and
      set the smart average=thredshold, stdvar=0.
     */
    ngx_str_t *value = cf->args->elts;
    ngx_int_t thredshold = ngx_atoi(value[1].data, value[1].len); /* the initial threadshold */
    PRINT_INT("init thredshold:", thredshold);
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

      NOTE: as it's a approx approach to calculating stdvar.
    */
    float stdvar=smart->stdvar;
    float aver=smart->average;
    aver = aver - ((float)smart->tail_weight / RING_SIZE) + ((float)weight / RING_SIZE); /* this is SMA */
    stdvar = stdvar + (POW2(weight - aver) -
                       POW2(smart->tail_weight - smart->average)) / RING_SIZE;           /* recalculate stdvar roughly, not precise */
    smart->stdvar = stdvar;
    smart->average = aver;
    smart->hist_weight[smart->tail_pos] = weight;
    smart->tail_pos = (smart->tail_pos + 1) % RING_SIZE;                                 /* this is how ring ptr move */
    smart->tail_weight = smart->hist_weight[smart->tail_pos];
    return stdvar;
}

static float Q_rsqrt( float number ) {
    /* the magic fast sqrt function from Quake 3*/
    long i;
    float x2, y;
    const float threehalfs = 1.5F;
    x2 = number * 0.5F;
    y  = number;
    i  = * ( long * ) &y;                       // evil floating point bit level hacking
    i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
    y  = * ( float * ) &i;
    y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
    //y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed
    return y;
}
