






































import BaseHTTPServer
import SimpleHTTPServer
import threading
import sys
import os
import urllib
import re
from urlparse import urlparse
from SocketServer import ThreadingMixIn

DOCROOT = '.'

class EasyServer(ThreadingMixIn, BaseHTTPServer.HTTPServer):
    allow_reuse_address = True
    
class MozRequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
    def translate_path(self, path):
        
        o = urlparse(path)
        return "%s%s" % ('' if sys.platform == 'win32' else '/', '/'.join([i.strip('/') for i in (DOCROOT, o.path)]))

    
    
    def address_string(self):
        return "a.b.c.d"

    
    def log_message(self, format, *args):
        pass

class MozHttpd(object):
    def __init__(self, host="127.0.0.1", port=8888, docroot='.'):
        global DOCROOT
        self.host = host
        self.port = int(port)
        DOCROOT = docroot

    def start(self):
        self.httpd = EasyServer((self.host, self.port), MozRequestHandler)
        self.server = threading.Thread(target=self.httpd.serve_forever)
        self.server.setDaemon(True) 
        self.server.start()
        

    
    def testServer(self):
        fileList = os.listdir(DOCROOT)
        filehandle = urllib.urlopen('http://%s:%s' % (self.host, self.port))
        data = filehandle.readlines();
        filehandle.close()

        for line in data:
            found = False
            
            webline = re.sub('\<[a-zA-Z0-9\-\_\.\=\"\'\/\\\%\!\@\#\$\^\&\*\(\) ]*\>', '', line.strip('\n')).strip('/').strip().strip('@')
            if webline != "":
                if webline == "Directory listing for":
                    found = True
                else:
                    for fileName in fileList:
                        if fileName == webline:
                            found = True
                
                if (found == False):
                    print "NOT FOUND: " + webline.strip()                

    def stop(self):
        if self.httpd:
            self.httpd.shutdown()
        
    __del__ = stop

