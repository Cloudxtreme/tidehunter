#!/usr/bin/env python2
# encoding: utf-8

import os
import tornado.web
import tornado.ioloop

class MainHandler(tornado.web.RequestHandler):
    def get(self):
        self.render("index.html")

class XssHandler(tornado.web.RequestHandler):
    def get(self):
        qstr = self.get_argument("qstr", default=None);
        self.render("xss.html", qstr=qstr)

class SqlHandler(tornado.web.RequestHandler):
    def get(self):
        qid = self.get_argument("id");
        self.render("sql.html", qid=qid)

if __name__ == "__main__":
    settings = {
            "autoescape" : None,
            "template_path" : os.path.join(os.path.dirname(__file__), "html"),
            }
    application = tornado.web.Application([
        (r"/", MainHandler),
        (r"/xss", XssHandler),
        (r"/sql", SqlHandler),
        ],
        **settings)
    application.listen(8086)
    tornado.ioloop.IOLoop.instance().start()
