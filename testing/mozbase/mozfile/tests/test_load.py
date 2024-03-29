

"""
tests for mozfile.load
"""

import mozhttpd
import os
import tempfile
import unittest
from mozfile import load


class TestLoad(unittest.TestCase):
    """test the load function"""

    def test_http(self):
        """test with mozhttpd and a http:// URL"""

        def example(request):
            """example request handler"""
            body = 'example'
            return (200, {'Content-type': 'text/plain',
                          'Content-length': len(body)
                          }, body)

        host = '127.0.0.1'
        httpd = mozhttpd.MozHttpd(host=host,
                                  urlhandlers=[{'method': 'GET',
                                                'path': '.*',
                                                'function': example}])
        try:
            httpd.start(block=False)
            content = load(httpd.get_url()).read()
            self.assertEqual(content, 'example')
        finally:
            httpd.stop()

    def test_file_path(self):
        """test loading from file path"""
        try:
            
            tmp = tempfile.NamedTemporaryFile(delete=False)
            tmp.write('foo bar')
            tmp.close()

            
            contents = file(tmp.name).read()
            self.assertEqual(contents, 'foo bar')

            
            self.assertEqual(load(tmp.name).read(), contents)

            
            self.assertEqual(load('file://%s' % tmp.name).read(), contents)
        finally:
            
            if os.path.exists(tmp.name):
                os.remove(tmp.name)

if __name__ == '__main__':
    unittest.main()
