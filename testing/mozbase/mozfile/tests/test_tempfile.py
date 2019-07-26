





"""
tests for mozfile.NamedTemporaryFile
"""

import mozfile
import os
import unittest


class TestNamedTemporaryFile(unittest.TestCase):
    """test our fix for NamedTemporaryFile"""

    def test_named_temporary_file(self):
        """ Ensure the fix for re-opening a NamedTemporaryFile works

            Refer to https://bugzilla.mozilla.org/show_bug.cgi?id=818777
            and https://bugzilla.mozilla.org/show_bug.cgi?id=821362
        """

        test_string = "A simple test"
        with mozfile.NamedTemporaryFile() as temp:
            
            temp.write(test_string)
            
            temp.flush()

            
            self.assertEqual(open(temp.name).read(), test_string)

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
