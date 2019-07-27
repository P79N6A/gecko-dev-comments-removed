































"""Tests for handshake._base module."""


import unittest

import set_sys_path  

from mod_pywebsocket.common import ExtensionParameter
from mod_pywebsocket.common import ExtensionParsingException
from mod_pywebsocket.common import format_extensions
from mod_pywebsocket.common import parse_extensions
from mod_pywebsocket.handshake._base import HandshakeException
from mod_pywebsocket.handshake._base import validate_subprotocol


class ValidateSubprotocolTest(unittest.TestCase):
    """A unittest for validate_subprotocol method."""

    def test_validate_subprotocol(self):
        
        validate_subprotocol('sample')
        validate_subprotocol('Sample')
        validate_subprotocol('sample\x7eprotocol')

        
        self.assertRaises(HandshakeException,
                          validate_subprotocol,
                          '')
        self.assertRaises(HandshakeException,
                          validate_subprotocol,
                          'sample\x09protocol')
        self.assertRaises(HandshakeException,
                          validate_subprotocol,
                          'sample\x19protocol')
        self.assertRaises(HandshakeException,
                          validate_subprotocol,
                          'sample\x20protocol')
        self.assertRaises(HandshakeException,
                          validate_subprotocol,
                          'sample\x7fprotocol')
        self.assertRaises(HandshakeException,
                          validate_subprotocol,
                          
                          u'\u65e5\u672c')


_TEST_TOKEN_EXTENSION_DATA = [
    ('foo', [('foo', [])]),
    ('foo; bar', [('foo', [('bar', None)])]),
    ('foo; bar=baz', [('foo', [('bar', 'baz')])]),
    ('foo; bar=baz; car=cdr', [('foo', [('bar', 'baz'), ('car', 'cdr')])]),
    ('foo; bar=baz, car; cdr',
     [('foo', [('bar', 'baz')]), ('car', [('cdr', None)])]),
    ('a, b, c, d',
     [('a', []), ('b', []), ('c', []), ('d', [])]),
    ]


_TEST_QUOTED_EXTENSION_DATA = [
    ('foo; bar=""', [('foo', [('bar', '')])]),
    ('foo; bar=" baz "', [('foo', [('bar', ' baz ')])]),
    ('foo; bar=",baz;"', [('foo', [('bar', ',baz;')])]),
    ('foo; bar="\\\r\\\nbaz"', [('foo', [('bar', '\r\nbaz')])]),
    ('foo; bar="\\"baz"', [('foo', [('bar', '"baz')])]),
    ('foo; bar="\xbbbaz"', [('foo', [('bar', '\xbbbaz')])]),
    ]


_TEST_REDUNDANT_TOKEN_EXTENSION_DATA = [
    ('foo \t ', [('foo', [])]),
    ('foo; \r\n bar', [('foo', [('bar', None)])]),
    ('foo; bar=\r\n \r\n baz', [('foo', [('bar', 'baz')])]),
    ('foo ;bar = baz ', [('foo', [('bar', 'baz')])]),
    ('foo,bar,,baz', [('foo', []), ('bar', []), ('baz', [])]),
    ]


_TEST_REDUNDANT_QUOTED_EXTENSION_DATA = [
    ('foo; bar="\r\n \r\n baz"', [('foo', [('bar', '  baz')])]),
    ]


class ExtensionsParserTest(unittest.TestCase):

    def _verify_extension_list(self, expected_list, actual_list):
        """Verifies that ExtensionParameter objects in actual_list have the
        same members as extension definitions in expected_list. Extension
        definition used in this test is a pair of an extension name and a
        parameter dictionary.
        """

        self.assertEqual(len(expected_list), len(actual_list))
        for expected, actual in zip(expected_list, actual_list):
            (name, parameters) = expected
            self.assertEqual(name, actual._name)
            self.assertEqual(parameters, actual._parameters)

    def test_parse(self):
        for formatted_string, definition in _TEST_TOKEN_EXTENSION_DATA:
            self._verify_extension_list(
                definition, parse_extensions(formatted_string,
                                             allow_quoted_string=False))

        for formatted_string, unused_definition in _TEST_QUOTED_EXTENSION_DATA:
            self.assertRaises(
                ExtensionParsingException, parse_extensions,
                formatted_string, False)

    def test_parse_with_allow_quoted_string(self):
        for formatted_string, definition in _TEST_TOKEN_EXTENSION_DATA:
            self._verify_extension_list(
                definition, parse_extensions(formatted_string,
                                             allow_quoted_string=True))

        for formatted_string, definition in _TEST_QUOTED_EXTENSION_DATA:
            self._verify_extension_list(
                definition, parse_extensions(formatted_string,
                                             allow_quoted_string=True))

    def test_parse_redundant_data(self):
        for (formatted_string,
             definition) in _TEST_REDUNDANT_TOKEN_EXTENSION_DATA:
            self._verify_extension_list(
                definition, parse_extensions(formatted_string,
                                             allow_quoted_string=False))

        for (formatted_string,
             definition) in _TEST_REDUNDANT_QUOTED_EXTENSION_DATA:
            self.assertRaises(
                ExtensionParsingException, parse_extensions,
                formatted_string, False)

    def test_parse_redundant_data_with_allow_quoted_string(self):
        for (formatted_string,
             definition) in _TEST_REDUNDANT_TOKEN_EXTENSION_DATA:
            self._verify_extension_list(
                definition, parse_extensions(formatted_string,
                                             allow_quoted_string=True))

        for (formatted_string,
             definition) in _TEST_REDUNDANT_QUOTED_EXTENSION_DATA:
            self._verify_extension_list(
                definition, parse_extensions(formatted_string,
                                             allow_quoted_string=True))

    def test_parse_bad_data(self):
        _TEST_BAD_EXTENSION_DATA = [
            ('foo; ; '),
            ('foo; a a'),
            ('foo foo'),
            (',,,'),
            ('foo; bar='),
            ('foo; bar="hoge'),
            ('foo; bar="a\r"'),
            ('foo; bar="\\\xff"'),
            ('foo; bar=\ra'),
            ]

        for formatted_string in _TEST_BAD_EXTENSION_DATA:
            self.assertRaises(
                ExtensionParsingException, parse_extensions, formatted_string)


class FormatExtensionsTest(unittest.TestCase):

    def test_format_extensions(self):
        for formatted_string, definitions in _TEST_TOKEN_EXTENSION_DATA:
            extensions = []
            for definition in definitions:
                (name, parameters) = definition
                extension = ExtensionParameter(name)
                extension._parameters = parameters
                extensions.append(extension)
            self.assertEqual(
                formatted_string, format_extensions(extensions))


if __name__ == '__main__':
    unittest.main()



