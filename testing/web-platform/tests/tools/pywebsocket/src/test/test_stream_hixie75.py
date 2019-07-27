































"""Tests for stream module."""


import unittest

import set_sys_path  

from mod_pywebsocket.stream import StreamHixie75
from test.test_msgutil import _create_request_hixie75


class StreamHixie75Test(unittest.TestCase):
    """A unittest for StreamHixie75 class."""

    def test_payload_length(self):
        for length, bytes in ((0, '\x00'), (0x7f, '\x7f'), (0x80, '\x81\x00'),
                              (0x1234, '\x80\xa4\x34')):
            test_stream = StreamHixie75(_create_request_hixie75(bytes))
            self.assertEqual(
                length, test_stream._read_payload_length_hixie75())


if __name__ == '__main__':
    unittest.main()



