































"""Tests for http_header_util module."""


import unittest

from mod_pywebsocket import http_header_util


class UnitTest(unittest.TestCase):
    """A unittest for http_header_util module."""

    def test_parse_relative_uri(self):
        host, port, resource = http_header_util.parse_uri('/ws/test')
        self.assertEqual(None, host)
        self.assertEqual(None, port)
        self.assertEqual('/ws/test', resource)

    def test_parse_absolute_uri(self):
        host, port, resource = http_header_util.parse_uri(
            'ws://localhost:10080/ws/test')
        self.assertEqual('localhost', host)
        self.assertEqual(10080, port)
        self.assertEqual('/ws/test', resource)

        host, port, resource = http_header_util.parse_uri(
            'ws://example.com/ws/test')
        self.assertEqual('example.com', host)
        self.assertEqual(80, port)
        self.assertEqual('/ws/test', resource)

        host, port, resource = http_header_util.parse_uri(
            'wss://example.com/')
        self.assertEqual('example.com', host)
        self.assertEqual(443, port)
        self.assertEqual('/', resource)

        host, port, resource = http_header_util.parse_uri(
            'ws://example.com:8080')
        self.assertEqual('example.com', host)
        self.assertEqual(8080, port)
        self.assertEqual('/', resource)

    def test_parse_invalid_uri(self):
        host, port, resource = http_header_util.parse_uri('ws:///')
        self.assertEqual(None, resource)

        host, port, resource = http_header_util.parse_uri('ws://localhost:')
        self.assertEqual(None, resource)

        host, port, resource = http_header_util.parse_uri('ws://localhost:/ws')
        self.assertEqual(None, resource)


if __name__ == '__main__':
    unittest.main()



