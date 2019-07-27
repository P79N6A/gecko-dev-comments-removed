































"""Tests for memorizingfile module."""


import StringIO
import unittest

import set_sys_path  

from mod_pywebsocket import memorizingfile


class UtilTest(unittest.TestCase):
    """A unittest for memorizingfile module."""

    def check(self, memorizing_file, num_read, expected_list):
        for unused in range(num_read):
            memorizing_file.readline()
        actual_list = memorizing_file.get_memorized_lines()
        self.assertEqual(len(expected_list), len(actual_list))
        for expected, actual in zip(expected_list, actual_list):
            self.assertEqual(expected, actual)

    def check_with_size(self, memorizing_file, read_size, expected_list):
        read_list = []
        read_line = ''
        while True:
            line = memorizing_file.readline(read_size)
            line_length = len(line)
            self.assertTrue(line_length <= read_size)
            if line_length == 0:
                if read_line != '':
                    read_list.append(read_line)
                break
            read_line += line
            if line[line_length - 1] == '\n':
                read_list.append(read_line)
                read_line = ''
        actual_list = memorizing_file.get_memorized_lines()
        self.assertEqual(len(expected_list), len(actual_list))
        self.assertEqual(len(expected_list), len(read_list))
        for expected, actual, read in zip(expected_list, actual_list,
                                          read_list):
            self.assertEqual(expected, actual)
            self.assertEqual(expected, read)

    def test_get_memorized_lines(self):
        memorizing_file = memorizingfile.MemorizingFile(StringIO.StringIO(
                'Hello\nWorld\nWelcome'))
        self.check(memorizing_file, 3, ['Hello\n', 'World\n', 'Welcome'])

    def test_get_memorized_lines_limit_memorized_lines(self):
        memorizing_file = memorizingfile.MemorizingFile(StringIO.StringIO(
                'Hello\nWorld\nWelcome'), 2)
        self.check(memorizing_file, 3, ['Hello\n', 'World\n'])

    def test_get_memorized_lines_empty_file(self):
        memorizing_file = memorizingfile.MemorizingFile(StringIO.StringIO(
                ''))
        self.check(memorizing_file, 10, [])

    def test_get_memorized_lines_with_size(self):
        for size in range(1, 10):
            memorizing_file = memorizingfile.MemorizingFile(StringIO.StringIO(
                'Hello\nWorld\nWelcome'))
            self.check_with_size(memorizing_file, size,
                                 ['Hello\n', 'World\n', 'Welcome'])

if __name__ == '__main__':
    unittest.main()



