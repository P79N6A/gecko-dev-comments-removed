





"""
tests for mozfile.TemporaryDirectory
"""

from mozfile import TemporaryDirectory
import os
import unittest


class TestTemporaryDirectory(unittest.TestCase):

    def test_removed(self):
        """ensure that a TemporaryDirectory gets removed"""
        path = None
        with TemporaryDirectory() as tmp:
            path = tmp
            self.assertTrue(os.path.isdir(tmp))
            tmpfile = os.path.join(tmp, "a_temp_file")
            open(tmpfile, "w").write("data")
            self.assertTrue(os.path.isfile(tmpfile))
        self.assertFalse(os.path.isdir(path))
        self.assertFalse(os.path.exists(path))

    def test_exception(self):
        """ensure that TemporaryDirectory handles exceptions"""
        path = None
        with self.assertRaises(Exception):
            with TemporaryDirectory() as tmp:
                path = tmp
                self.assertTrue(os.path.isdir(tmp))
                raise Exception("oops")
        self.assertFalse(os.path.isdir(path))
        self.assertFalse(os.path.exists(path))

if __name__ == '__main__':
    unittest.main()
