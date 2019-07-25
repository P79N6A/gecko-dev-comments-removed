






































import BaseHTTPServer
import SimpleHTTPServer
import threading
import sys
import os
import urllib
import re
from SocketServer import ThreadingMixIn

class EasyServer(ThreadingMixIn, BaseHTTPServer.HTTPServer):
    allow_reuse_address = True
    
class MozRequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
    docroot = os.getcwd()

    def parse_request(self):
        retval = SimpleHTTPServer.SimpleHTTPRequestHandler.parse_request(self)
        if '?' in self.path:
            
            
            self.path = self.path.split('?', 1)[0]
        return retval

    def translate_path(self, path):
        path = path.strip('/').split()
        if path == ['']:
            path = []
        path.insert(0, self.docroot)
        return os.path.join(*path)

    
    
    def address_string(self):
        return "a.b.c.d"

    
    def log_message(self, format, *args):
        pass

class MozHttpd(object):

    def __init__(self, host="127.0.0.1", port=8888, docroot=os.getcwd()):
        self.host = host
        self.port = int(port)
        self.docroot = docroot
        self.httpd = None

    def start(self, block=False):
        """
        start the server.  If block is True, the call will not return.
        If block is False, the server will be started on a separate thread that
        can be terminated by a call to .stop()
        """

        class MozRequestHandlerInstance(MozRequestHandler):
            docroot = self.docroot

        self.httpd = EasyServer((self.host, self.port), MozRequestHandlerInstance)
        if block:
            self.httpd.serve_forever()
        else:
            self.server = threading.Thread(target=self.httpd.serve_forever)
            self.server.setDaemon(True) 
            self.server.start()
        
    def testServer(self):
        fileList = os.listdir(self.docroot)
        filehandle = urllib.urlopen('http://%s:%s/?foo=bar&fleem=&foo=fleem' % (self.host, self.port))
        data = filehandle.readlines()
        filehandle.close()

        retval = True

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
                
                if not found:
                    retval = False
                    print >> sys.stderr, "NOT FOUND: " + webline.strip()
        return retval

    def stop(self):
        if self.httpd:
            self.httpd.shutdown()
        self.httpd = None

    __del__ = stop


def main(args=sys.argv[1:]):
    
    
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option('-p', '--port', dest='port', 
                      type="int", default=8888,
                      help="port to run the server on [DEFAULT: %default]")
    parser.add_option('-H', '--host', dest='host',
                      default='127.0.0.1',
                      help="host [DEFAULT: %default]")
    parser.add_option('-d', '--docroot', dest='docroot',
                      default=os.getcwd(),
                      help="directory to serve files from [DEFAULT: %default]")
    parser.add_option('--test', dest='test',
                      action='store_true', default=False,
                      help='run the tests and exit')
    options, args = parser.parse_args(args)
    if args:
        parser.print_help()
        parser.exit()

    
    kwargs = options.__dict__.copy()
    test = kwargs.pop('test')
    server = MozHttpd(**kwargs)

    if test:
        server.start()
        server.testServer()
    else:
        server.start(block=True)

if __name__ == '__main__':
    main()
