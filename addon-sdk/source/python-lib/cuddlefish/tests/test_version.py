



import os
import unittest
import shutil

from cuddlefish._version import get_versions

class Version(unittest.TestCase):
    def get_basedir(self):
        return os.path.join(".test_tmp", self.id())
    def make_basedir(self):
        basedir = self.get_basedir()
        if os.path.isdir(basedir):
            here = os.path.abspath(os.getcwd())
            assert os.path.abspath(basedir).startswith(here) 
            shutil.rmtree(basedir)
        os.makedirs(basedir)
        return basedir

    def test_current_version(self):
        
        
        version = get_versions()["version"]
        self.failUnless(isinstance(version, str), (version, type(version)))
        self.failUnless(len(version) > 0, version)
