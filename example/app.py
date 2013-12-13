#!/usr/bin/env python2
# encoding: utf-8

import os
import tornado.web
import tornado.ioloop

import database

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
        rows = database.query(qid);
        if len(rows) == 0:
            self.write("id error")
            return
        info = rows[0][1]
        self.render("sql.html", info=info)

if __name__ == "__main__":
    settings = {
            "autoescape" : None,    #so to make xss available
            "template_path" : os.path.join(os.path.dirname(__file__), "html"),
            "debug" : True,
            }
    application = tornado.web.Application([
        (r"/", MainHandler),
        (r"/xss", XssHandler),
        (r"/sql", SqlHandler),
        ],
        **settings)
    application.listen(8086)
    tornado.ioloop.IOLoop.instance().start()
