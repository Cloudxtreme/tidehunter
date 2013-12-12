## Intro

TideHunter(aka. TH) borrow some ideas from [naxsi](https://github.com/nbs-system/naxsi).
Basicly, TH try to filter on all the requests at nginx `HTTP\_REWRITE\_PAHSE`.

Let's look at a basic nginx.conf for TH first:

    http {
        .... #not listed

        tidehunter\_loadrule\_qstr /opt/nginx/qstr.json;
        server {
            listen 80;
            location / {
                proxy_pass http://localhost:8080;
                tidehunter\_smart\_thredshold 16;
            }
        }
    }

The filters' rules is loaded by the nginx directive `tidehunter\_loadrule\_xxxx`.
for example, I can load the rule for query string filter like thie,

    tidehunter\_loadrule_qstr /opt/nginx/rules/qstr.json

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
`tidehunter\_smart\_thredshold` or `tidehunter\_static\_thredshold` is set
in nginx location block.

the `tidehunter\_smart\_thredshold` is the main feature that differs from naxsi.
when set, the module activate the smart learning mode to record the most recent
10000 request's weight, and it calculate the average and standard variance(stdvar) of the weights.
(the amount of most recent requests is configurable at compile time.)

when new request comes in, the weight of the request is compare to the sum of `average` and `stdvar`,
and if the weight is greater, then the request is BLOCKED.

every time a request arrives, the `average` and `stdvar` are updated in a approx way(so to be fast).
by this method, TH can block abnormal request automaticly.


## The Filter

Filter can take place at request's query-string, uri, body, cookie(not yet) etc.
To load rule for query-string filter, apply this directive command in nginx.conf's http block:
`tidehunter\_loadrule\_qstr $your_rule_file_path`.

_NOTE_: the uri filter is quite different from other filters, the uri filter is designed to be a
white/black list filter. that is,

- when you don't want to have specific uri blocked, you write a uri filter
rule for that uri, and set the weight to be `negative`.
- when you want to increase the chance that specific uri get blocked, you write a uri filter
rule for that uri, and set the weight to be a possitive weight, so it add up to the weight
calculated by other filters.
