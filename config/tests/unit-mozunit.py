



import sys
import os
from mozunit import main, MockedOpen
import unittest
from tempfile import mkstemp

class TestMozUnit(unittest.TestCase):
    def test_mocked_open(self):
        
        (fd, path) = mkstemp()
        with os.fdopen(fd, 'w') as file:
            file.write('foobar');

        self.assertFalse(os.path.exists('file1'))
        self.assertFalse(os.path.exists('file2'))

        with MockedOpen({'file1': 'content1',
                         'file2': 'content2'}):
            self.assertTrue(os.path.exists('file1'))
            self.assertTrue(os.path.exists('file2'))
            self.assertFalse(os.path.exists('foo/file1'))

            
            self.assertEqual(open('file1', 'r').read(), 'content1')
            self.assertEqual(open('file2', 'r').read(), 'content2')

            
            with open('file1', 'w') as file:
                file.write('foo')
            self.assertTrue(os.path.exists('file1'))
            self.assertEqual(open('file1', 'r').read(), 'foo')

            
            file = open('file2', 'w')
            file.write('bar')
            self.assertEqual(open('file2', 'r').read(), 'content2')
            file.close()
            self.assertEqual(open('file2', 'r').read(), 'bar')

            
            with open('file1', 'a') as file:
                file.write('bar')
            self.assertEqual(open('file1', 'r').read(), 'foobar')

            self.assertFalse(os.path.exists('file3'))

            
            self.assertRaises(IOError, open, 'file3', 'r')
            self.assertFalse(os.path.exists('file3'))

            
            with open('file3', 'w') as file:
                file.write('baz')
            self.assertEqual(open('file3', 'r').read(), 'baz')
            self.assertTrue(os.path.exists('file3'))

            
            self.assertEqual(open(path, 'r').read(), 'foobar')

            
            
            with open(path, 'w') as file:
                file.write('bazqux')
            self.assertEqual(open(path, 'r').read(), 'bazqux')

        with MockedOpen():
            
            
            with open(path, 'a') as file:
                file.write('bazqux')
            self.assertEqual(open(path, 'r').read(), 'foobarbazqux')

        
        self.assertEqual(open(path, 'r').read(), 'foobar')
        os.remove(path)

        
        
        self.assertRaises(IOError, open, 'file3', 'r')

if __name__ == "__main__":
    main()
