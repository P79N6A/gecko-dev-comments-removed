from marionette_test import MarionetteTestCase
import threading
import SimpleHTTPServer
import SocketServer
import BaseHTTPServer
import socket
import urllib
import urlparse
import os

DEBUG = False








class ThreadingSimpleServer(SocketServer.ThreadingMixIn,
                            BaseHTTPServer.HTTPServer):
    pass


class QuietHttpRequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
    def log_message(self, format, *args, **kwargs):
        pass


class BaseTestFrontendUnits(MarionetteTestCase):

    @classmethod
    def setUpClass(cls):
        super(BaseTestFrontendUnits, cls).setUpClass()

        if DEBUG:
            handler = SimpleHTTPServer.SimpleHTTPRequestHandler
        else:
            handler = QuietHttpRequestHandler

        
        cls.server = ThreadingSimpleServer(('', 0), handler)
        cls.ip, cls.port = cls.server.server_address

        cls.server_thread = threading.Thread(target=cls.server.serve_forever)
        cls.server_thread.daemon = False
        cls.server_thread.start()

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()
        cls.server_thread.join()

        
        
        
        cls.server_thread = None
        cls.server = None

    def setUp(self):
        super(BaseTestFrontendUnits, self).setUp()

        
        
        self.marionette.set_search_timeout(60000)

    
    def set_server_prefix(self, srcdir_path):
        
        
        

        
        commonPath = os.path.commonprefix([__file__, os.getcwd()])

        
        self.relPath = os.path.relpath(os.path.dirname(__file__), commonPath)

        self.relPath = urllib.pathname2url(os.path.join(self.relPath, srcdir_path))

        
        self.server_prefix = urlparse.urljoin("http://localhost:" + str(self.port),
                                              self.relPath)

    def check_page(self, page):

        self.marionette.navigate(urlparse.urljoin(self.server_prefix, page))
        self.marionette.find_element("id", 'complete')

        fail_node = self.marionette.find_element("css selector",
                                                 '.failures > em')
        if fail_node.text == "0":
            return

        
        
        
        
        
        
        
        
        

        raise AssertionError(self.get_failure_details(page))

    def get_failure_details(self, page):
        fail_nodes = self.marionette.find_elements("css selector",
                                                   '.test.fail')
        fullPageUrl = urlparse.urljoin(self.relPath, page)

        details = ["%s: %d failure(s) encountered:" % (fullPageUrl, len(fail_nodes))]

        for node in fail_nodes:
            errorText = node.find_element("css selector", '.error').text

            
            
            

            
            details.append(
                "TEST-UNEXPECTED-FAIL | %s | %s - %s" % \
                (fullPageUrl, node.find_element("tag name", 'h2').text.split("\n")[0], errorText.split("\n")[0]))
            details.append(
                errorText)
        return "\n".join(details)
