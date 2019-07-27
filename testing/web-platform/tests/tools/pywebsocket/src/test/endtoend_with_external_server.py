































"""Test for end-to-end with external server.

This test is not run by run_all.py because it requires some preparations.
If you would like to run this test correctly, launch Apache with mod_python
and mod_pywebsocket manually. In addition, you should pass allow_draft75 option
and example path as handler_scan option and Apache's DocumentRoot.
"""


import optparse
import sys
import test.test_endtoend
import unittest


_DEFAULT_WEB_SOCKET_PORT = 80


class EndToEndTestWithExternalServer(test.test_endtoend.EndToEndTest):
    pass

if __name__ == '__main__':
    parser = optparse.OptionParser()
    parser.add_option('-p', '--port', dest='port', type='int',
                      default=_DEFAULT_WEB_SOCKET_PORT,
                      help='external test server port.')
    (options, args) = parser.parse_args()

    test.test_endtoend._use_external_server = True
    test.test_endtoend._external_server_port = options.port

    unittest.main(argv=[sys.argv[0]])



