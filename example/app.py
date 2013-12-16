#!/usr/bin/env python2
# encoding: utf-8

# this is a web server full of vulnerabilities
# XSS, SQL injection, Directory Traversal, PHP file include.
#
# unfaithfully by ruoshan.
#
# INSTALLATION: 
#   - require sqlite3 and pysqlite for SQL
#   - require php for PHP file include

import os
import urllib2
import subprocess
from subprocess import PIPE
import tornado.web
import tornado.ioloop

import database

class MainHandler(tornado.web.RequestHandler):
    def get(self):
        self.render("index.html")

class XssHandler(tornado.web.RequestHandler):
    # xss attack
    def get(self):
        qstr = self.get_argument("qstr", default=None);
        self.render("xss.html", qstr=qstr)

class SqlHandler(tornado.web.RequestHandler):
    # sql injection
    def get(self):
        qid = self.get_argument("id");
        rows = database.query(qid);
        if len(rows) == 0:
            self.write("id error")
            return
        info = rows[0][1]
        self.render("sql.html", info=info)

class LfiHandler(tornado.web.RequestHandler):
    # Local File Inclusion
    def get(self):
        try:
            filename = self.get_argument("filename")
        except:
            self.render("lfi.html")
            return
        if os.path.exists(filename) and not os.path.isdir(filename):
            self.write(open(filename, 'r').read())     #give you all the secrets
        else:
            raise tornado.web.HTTPError(400)

    def post(self):
        self.get()

class PhpHandler(tornado.web.RequestHandler):
    # Remote File Inclusion, (php)
    # FIXME: NOT work, php under cli is diff from php behind cgi
    def get(self):
        # php file on remote side
        try:
            phpurl = self.get_argument("phpurl")
        except:
            self.render("php.html")
            return
        phpcode = "<?php include(\"%s\") ?>" % phpurl
        print phpcode
        p = subprocess.Popen(["php", ], stdin=PIPE, stdout=PIPE, stderr=PIPE)
        stdout, _ = p.communicate(input=phpcode)
        self.write(stdout)

    def post(self):
        self.get()

class UploadHandler(tornado.web.RequestHandler):
    # not used yet
    def post(self):
        # execute php code in upload form
        files = self.request.files
        for var in files:
            for httpfile in files[var]:
                p = subprocess.Popen(["php", ], stdin=PIPE, stdout=PIPE, stderr=PIPE)
                stdout, _ = p.communicate(input=httpfile.body)
                self.write(stdout)

class PhpStrHandler(tornado.web.RequestHandler):
    def get(self):
        self.write("<?php phpinfo() ?>")

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
        (r"/php", PhpHandler),
        (r"/phpstr", PhpStrHandler),
        (r"/lfi", LfiHandler),
        ],
        **settings)
    application.listen(8086)
    tornado.ioloop.IOLoop.instance().start()
