































"""Tests for util module."""


import os
import random
import sys
import unittest

import set_sys_path  

from mod_pywebsocket import util


_TEST_DATA_DIR = os.path.join(os.path.split(__file__)[0], 'testdata')


class UtilTest(unittest.TestCase):
    """A unittest for util module."""

    def test_get_stack_trace(self):
        self.assertEqual('None\n', util.get_stack_trace())
        try:
            a = 1 / 0  
        except Exception:
            trace = util.get_stack_trace()
            self.failUnless(trace.startswith('Traceback'))
            self.failUnless(trace.find('ZeroDivisionError') != -1)

    def test_prepend_message_to_exception(self):
        exc = Exception('World')
        self.assertEqual('World', str(exc))
        util.prepend_message_to_exception('Hello ', exc)
        self.assertEqual('Hello World', str(exc))

    def test_get_script_interp(self):
        cygwin_path = 'c:\\cygwin\\bin'
        cygwin_perl = os.path.join(cygwin_path, 'perl')
        self.assertEqual(None, util.get_script_interp(
            os.path.join(_TEST_DATA_DIR, 'README')))
        self.assertEqual(None, util.get_script_interp(
            os.path.join(_TEST_DATA_DIR, 'README'), cygwin_path))
        self.assertEqual('/usr/bin/perl -wT', util.get_script_interp(
            os.path.join(_TEST_DATA_DIR, 'hello.pl')))
        self.assertEqual(cygwin_perl + ' -wT', util.get_script_interp(
            os.path.join(_TEST_DATA_DIR, 'hello.pl'), cygwin_path))

    def test_hexify(self):
        self.assertEqual('61 7a 41 5a 30 39 20 09 0d 0a 00 ff',
                         util.hexify('azAZ09 \t\r\n\x00\xff'))


class RepeatedXorMaskerTest(unittest.TestCase):
    """A unittest for RepeatedXorMasker class."""

    def test_mask(self):
        
        masker = util.RepeatedXorMasker('\xff\xff\xff\xff')
        result = masker.mask('\xe6\x97\xa5')
        self.assertEqual('\x19\x68\x5a', result)

        masker = util.RepeatedXorMasker('\x00\x00\x00\x00')
        result = masker.mask('\xe6\x97\xa5')
        self.assertEqual('\xe6\x97\xa5', result)

        masker = util.RepeatedXorMasker('\xe6\x97\xa5\x20')
        result = masker.mask('\xe6\x97\xa5')
        self.assertEqual('\x00\x00\x00', result)

    def test_mask_twice(self):
        masker = util.RepeatedXorMasker('\x00\x7f\xff\x20')
        
        result = masker.mask('\x00\x00\x00\x00\x00')
        self.assertEqual('\x00\x7f\xff\x20\x00', result)
        
        result = masker.mask('\x00\x00\x00\x00\x00')
        self.assertEqual('\x7f\xff\x20\x00\x7f', result)

    def test_mask_large_data(self):
        masker = util.RepeatedXorMasker('mASk')
        original = ''.join([chr(i % 256) for i in xrange(1000)])
        result = masker.mask(original)
        expected = ''.join(
                [chr((i % 256) ^ ord('mASk'[i % 4])) for i in xrange(1000)])
        self.assertEqual(expected, result)

        masker = util.RepeatedXorMasker('MaSk')
        first_part = 'The WebSocket Protocol enables two-way communication.'
        result = masker.mask(first_part)
        self.assertEqual(
                '\x19\t6K\x1a\x0418"\x028\x0e9A\x03\x19"\x15<\x08"\rs\x0e#'
                '\x001\x07(\x12s\x1f:\x0e~\x1c,\x18s\x08"\x0c>\x1e#\x080\n9'
                '\x08<\x05c',
                result)
        second_part = 'It has two parts: a handshake and the data transfer.'
        result = masker.mask(second_part)
        self.assertEqual(
                "('K%\x00 K9\x16<K=\x00!\x1f>[s\nm\t2\x05)\x12;\n&\x04s\n#"
                "\x05s\x1f%\x04s\x0f,\x152K9\x132\x05>\x076\x19c",
                result)


def get_random_section(source, min_num_chunks):
    chunks = []
    bytes_chunked = 0

    while bytes_chunked < len(source):
        chunk_size = random.randint(
            1,
            min(len(source) / min_num_chunks, len(source) - bytes_chunked))
        chunk = source[bytes_chunked:bytes_chunked + chunk_size]
        chunks.append(chunk)
        bytes_chunked += chunk_size

    return chunks


class InflaterDeflaterTest(unittest.TestCase):
    """A unittest for _Inflater and _Deflater class."""

    def test_inflate_deflate_default(self):
        input = b'hello' + '-' * 30000 + b'hello'
        inflater15 = util._Inflater(15)
        deflater15 = util._Deflater(15)
        inflater8 = util._Inflater(8)
        deflater8 = util._Deflater(8)

        compressed15 = deflater15.compress_and_finish(input)
        compressed8 = deflater8.compress_and_finish(input)

        inflater15.append(compressed15)
        inflater8.append(compressed8)

        self.assertNotEqual(compressed15, compressed8)
        self.assertEqual(input, inflater15.decompress(-1))
        self.assertEqual(input, inflater8.decompress(-1))

    def test_random_section(self):
        random.seed(a=0)
        source = ''.join(
            [chr(random.randint(0, 255)) for i in xrange(100 * 1024)])

        chunked_input = get_random_section(source, 10)
        print "Input chunk sizes: %r" % [len(c) for c in chunked_input]

        deflater = util._Deflater(15)
        compressed = []
        for chunk in chunked_input:
            compressed.append(deflater.compress(chunk))
        compressed.append(deflater.compress_and_finish(''))

        chunked_expectation = get_random_section(source, 10)
        print ("Expectation chunk sizes: %r" %
               [len(c) for c in chunked_expectation])

        inflater = util._Inflater(15)
        inflater.append(''.join(compressed))
        for chunk in chunked_expectation:
            decompressed = inflater.decompress(len(chunk))
            self.assertEqual(chunk, decompressed)

        self.assertEqual('', inflater.decompress(-1))


if __name__ == '__main__':
    unittest.main()



