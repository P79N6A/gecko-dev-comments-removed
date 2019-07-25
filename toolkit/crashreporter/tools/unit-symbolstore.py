

import os, tempfile, unittest, shutil
import symbolstore

class TestGetVCSFilename(unittest.TestCase):
    """
    Unit tests for GetVCSFilename

    """
    def setUp(self):
        self.srcdir = tempfile.mkdtemp()
        
        os.mkdir(os.path.join(self.srcdir, ".hg"))
        
        symbolstore.HGRepoInfo.repos[self.srcdir] = \
            symbolstore.HGRepoInfo("http://example.com/repo",
                                   "abcd",
                                   'example.com/repo')
        objdir = os.path.join(self.srcdir, "obj")
        os.mkdir(objdir)
        self.oldcwd = os.getcwd()
        os.chdir(objdir)

    def tearDown(self):
        os.chdir(self.oldcwd)
        shutil.rmtree(self.srcdir)

    def testRelativePath(self):
        """
        Test that a relative path doesn't get
        removed.

        """
        vcsfile, root = symbolstore.GetVCSFilename("../foo.h", [self.srcdir])
        vcs,location,filename,rev = vcsfile.split(":")
        self.assertEqual(filename, "foo.h")

if __name__ == '__main__':
  unittest.main()
