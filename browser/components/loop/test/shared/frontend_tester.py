from marionette_test import MarionetteTestCase
import threading
import SimpleHTTPServer
import SocketServer
import BaseHTTPServer
import urlparse

PORT = 2222
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

        cls.server = ThreadingSimpleServer(('', PORT), handler)

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

    def set_server_prefix(self, srcdir_path=None):
        self.server_prefix = urlparse.urljoin("http://localhost:" + str(PORT),
                                              srcdir_path)

    def check_page(self, page):

        self.marionette.navigate(urlparse.urljoin(self.server_prefix, page))
        self.marionette.find_element("id", 'complete')

        fail_node = self.marionette.find_element("css selector",
                                                 '.failures > em')
        if fail_node.text == "0":
            return

        
        
        
        
        
        
        
        
        

        raise AssertionError(self.get_failure_details())

    def get_failure_details(self):
        fail_nodes = self.marionette.find_elements("css selector",
                                                   '.test.fail')
        details = ["%d failure(s) encountered:" % len(fail_nodes)]
        for node in fail_nodes:
            details.append(
                node.find_element("tag name", 'h2').text.split("\n")[0])
            details.append(
                node.find_element("css selector", '.error').text)
        return "\n".join(details)
