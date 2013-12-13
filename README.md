## Highlights

- automaticly adjust the filter's thredshold to your site
- whitelist-orient is also available
- super fast. writen in C, async processing request headers and request body.
- extensible. new filter, new rule format parser, etc. all can be easily integrated into module.
- SID. (aka. Still In Development)

## Intro

TideHunter(aka. TH) borrow some ideas from [naxsi](https://github.com/nbs-system/naxsi).
Basicly, TH try to filter on all the requests at nginx `HTTP_REWRITE_PAHSE`.

Let's look at a basic nginx.conf for TH first:

    http {
        .... #not listed

        tidehunter_loadrule_qstr /opt/nginx/qstr.json;
        server {
            listen 80;
            location / {
                proxy_pass http://localhost:8080;
                tidehunter_smart_thredshold 16;
            }
        }
    }

The filters' rules is loaded by the nginx directive `tidehunter_loadrule_xxxx`.
for example, I can load the rule for query string filter like thie,

    tidehunter_loadrule_qstr /opt/nginx/rules/qstr.json

and the content in `qstr.json` looks like:

    [{"msg" : "this is a query string filter",
      "id" : "1001",
      "weight" : 8,
      "match_opt" : 1,
      "regex_str" : "select|from|insert|drop|truncate"}]

rule file is a json list of objects. the `weight` member of json object is a
number showing how 'bad' the request is. weight is accumulated when multiple rules
are matched.

but TH won't block request that match the rules untill either the directives
`tidehunter_smart_thredshold` or `tidehunter_static_thredshold` is set
in nginx location block.

the `tidehunter_smart_thredshold` is the main feature that differs from naxsi.
when set, the module activate the smart learning mode to record the most recent
10000 request's weight, and it calculate the average and standard variance(stdvar) of the weights.
(the amount of most recent requests is configurable at compile time.)

when new request comes in, the weight of the request is compare to the sum of `average` and `stdvar`,
and if the weight is greater, then the request is BLOCKED.

every time a request arrives, the `average` and `stdvar` are updated in a approx way(so to be fast).
by this method, TH can block abnormal request automaticly.


## The Filter

Filter can take place at request's query-string, uri, body, cookie(not yet) etc.

- To load rule for query-string filter, apply this directive command in nginx.conf's http block:
`tidehunter_loadrule_qstr $your_rule_file_path`.
- To load rule for body filter, apply this directive command in nginx.conf's http block:
`tidehunter_loadrule_body $your_rule_file_path`.
- To load rule for uri filter, apply this directive command in nginx.conf's _location_ block:
`tidehunter_loadrule_uri $your_rule_file_path`.
- more filter to be implemented...

_NOTE_: the uri filter is quite different from other filters, the uri filter is designed to be a
white/black list filter. that is,

- when you don't want to have specific uri blocked, you write a uri filter
rule for that uri, and set the weight to be `negative`.
- when you want to increase the chance that specific uri get blocked, you write a uri filter
rule for that uri, and set the weight to be a possitive weight, so it add up to the weight
calculated by other filters.

## Installation

this module require jansson lib to parse json:

    apt-get install libjansson-dev

or, compile it yourself.

this module is tested on nginx-1.4.x. compile your nginx:

    cd nginx-src; ./configure --prefix=/opt/nginx --add-module=PATH_to_tidehunter

if you compile the jansson yourself, then you need to edit the nginx-src/objs/Makefile:

- add "-I PATH\_to\_jansson/include" to ALL_INCS var
- add "-L PATH\_to\_jansson/lib" to makefile target objs/nginx
- before run nginx, update your env `export LD_LIBRARY_PATH="LD_LIBRARY_PATH:PATH_to_jansson/lib"`
