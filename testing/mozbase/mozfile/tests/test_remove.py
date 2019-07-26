

import os
import stat
import shutil
import tempfile
import threading
import time
import unittest

import mozfile
import mozinfo

import stubs


def mark_readonly(path):
    """Removes all write permissions from given file/directory.

    :param path: path of directory/file of which modes must be changed
    """
    mode = os.stat(path)[stat.ST_MODE]
    os.chmod(path, mode & ~stat.S_IWUSR & ~stat.S_IWGRP & ~stat.S_IWOTH)


class FileOpenCloseThread(threading.Thread):
    """Helper thread for asynchronous file handling"""
    def __init__(self, path, delay, delete=False):
        threading.Thread.__init__(self)
        self.delay = delay
        self.path = path
        self.delete = delete

    def run(self):
        with open(self.path) as f:
            time.sleep(self.delay)
        if self.delete:
            try:
                os.remove(self.path)
            except:
                pass


class MozfileRemoveTestCase(unittest.TestCase):
    """Test our ability to remove directories and files"""

    def setUp(self):
        
        self.tempdir = stubs.create_stub()

    def tearDown(self):
        if os.path.isdir(self.tempdir):
            shutil.rmtree(self.tempdir)

    def test_remove_directory(self):
        """Test the removal of a directory"""
        self.assertTrue(os.path.isdir(self.tempdir))
        mozfile.remove(self.tempdir)
        self.assertFalse(os.path.exists(self.tempdir))

    def test_remove_directory_with_open_file(self):
        """Test removing a directory with an open file"""
        
        filepath = os.path.join(self.tempdir, *stubs.files[1])
        f = file(filepath, 'w')
        f.write('foo-bar')

        
        if mozinfo.isWin:
            
            self.assertRaises(OSError, mozfile.remove, self.tempdir)
            self.assertTrue(os.path.exists(self.tempdir))
        else:
            
            mozfile.remove(self.tempdir)
            self.assertFalse(os.path.exists(self.tempdir))

    def test_remove_closed_file(self):
        """Test removing a closed file"""
        
        filepath = os.path.join(self.tempdir, *stubs.files[1])
        with open(filepath, 'w') as f:
            f.write('foo-bar')

        
        mozfile.remove(self.tempdir)
        self.assertFalse(os.path.exists(self.tempdir))

    def test_removing_open_file_with_retry(self):
        """Test removing a file in use with retry"""
        filepath = os.path.join(self.tempdir, *stubs.files[1])

        thread = FileOpenCloseThread(filepath, 1)
        thread.start()

        
        time.sleep(.5)
        mozfile.remove(filepath)
        thread.join()

        
        self.assertFalse(os.path.exists(filepath))

    def test_removing_already_deleted_file_with_retry(self):
        """Test removing a meanwhile removed file with retry"""
        filepath = os.path.join(self.tempdir, *stubs.files[1])

        thread = FileOpenCloseThread(filepath, .8, True)
        thread.start()

        
        
        time.sleep(.5)
        mozfile.remove(filepath)
        thread.join()

        
        self.assertFalse(os.path.exists(filepath))

    def test_remove_readonly_tree(self):
        """Test removing a read-only directory"""

        dirpath = os.path.join(self.tempdir, "nested_tree")
        mark_readonly(dirpath)

        
        mozfile.remove(dirpath)

        self.assertFalse(os.path.exists(dirpath))

    def test_remove_readonly_file(self):
        """Test removing read-only files"""
        filepath = os.path.join(self.tempdir, *stubs.files[1])
        mark_readonly(filepath)

        
        mozfile.remove(filepath)

        self.assertFalse(os.path.exists(filepath))
