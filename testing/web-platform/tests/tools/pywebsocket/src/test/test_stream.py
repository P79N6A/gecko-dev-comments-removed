































"""Tests for stream module."""


import unittest

import set_sys_path  

from mod_pywebsocket import common
from mod_pywebsocket import stream


class StreamTest(unittest.TestCase):
    """A unittest for stream module."""

    def test_create_header(self):
        
        header = stream.create_header(common.OPCODE_TEXT, 1, 1, 1, 1, 1, 1)
        self.assertEqual('\xf1\x81', header)

        
        header = stream.create_header(
            common.OPCODE_TEXT, (1 << 63) - 1, 0, 0, 0, 0, 0)
        self.assertEqual('\x01\x7f\x7f\xff\xff\xff\xff\xff\xff\xff', header)

        
        self.assertRaises(ValueError,
                          stream.create_header,
                          0x10, 0, 0, 0, 0, 0, 0)

        
        self.assertRaises(ValueError,
                          stream.create_header,
                          common.OPCODE_TEXT, 0, 0xf, 0, 0, 0, 0)

        
        self.assertRaises(ValueError,
                          stream.create_header,
                          common.OPCODE_TEXT, 1 << 63, 0, 0, 0, 0, 0)


if __name__ == '__main__':
    unittest.main()



