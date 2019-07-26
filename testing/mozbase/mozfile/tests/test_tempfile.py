





"""
tests for mozfile.NamedTemporaryFile
"""

import mozfile
import os
import unittest

class TestNamedTemporaryFile(unittest.TestCase):

    def test_iteration(self):
        """ensure the line iterator works"""

        
        tf = mozfile.NamedTemporaryFile()
        notes = ['doe', 'rae', 'mi']
        for note in notes:
            tf.write('%s\n' % note)
        tf.flush()

        
        tf.seek(0)
        lines = [line.rstrip('\n') for line in tf.readlines()]
        self.assertEqual(lines, notes)

        
        lines = []
        for line in tf:
            lines.append(line.strip())
        self.assertEqual(lines, []) 
        tf.seek(0)
        lines = []
        for line in tf:
            lines.append(line.strip())
        self.assertEqual(lines, notes)

    def test_delete(self):
        """ensure ``delete=True/False`` works as expected"""

        
        path = None
        with mozfile.NamedTemporaryFile(delete=True) as tf:
            path = tf.name
        self.assertTrue(isinstance(path, basestring))
        self.assertFalse(os.path.exists(path))

        
        
        tf = mozfile.NamedTemporaryFile(delete=True)
        path = tf.name
        self.assertTrue(os.path.exists(path))
        del tf
        self.assertFalse(os.path.exists(path))

        
        path = None
        try:
            with mozfile.NamedTemporaryFile(delete=False) as tf:
                path = tf.name
            self.assertTrue(os.path.exists(path))
        finally:
            if path and os.path.exists(path):
                os.remove(path)

        path = None
        try:
            tf = mozfile.NamedTemporaryFile(delete=False)
            path = tf.name
            self.assertTrue(os.path.exists(path))
            del tf
            self.assertTrue(os.path.exists(path))
        finally:
            if path and os.path.exists(path):
                os.remove(path)

if __name__ == '__main__':
    unittest.main()
