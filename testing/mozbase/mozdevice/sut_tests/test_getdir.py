



import os
import posixpath
import shutil
import tempfile

from dmunit import DeviceManagerTestCase


class GetDirectoryTestCase(DeviceManagerTestCase):

    def _setUp(self):
        self.localsrcdir = tempfile.mkdtemp()
        os.makedirs(os.path.join(self.localsrcdir, 'push1', 'sub.1', 'sub.2'))
        path = os.path.join(self.localsrcdir,
                            'push1', 'sub.1', 'sub.2', 'testfile')
        file(path, 'w').close()
        os.makedirs(os.path.join(self.localsrcdir, 'push1', 'emptysub'))
        self.localdestdir = tempfile.mkdtemp()
        self.expected_filelist = ['emptysub', 'sub.1']

    def tearDown(self):
        shutil.rmtree(self.localsrcdir)
        shutil.rmtree(self.localdestdir)

    def runTest(self):
        """This tests the getDirectory() function.
        """
        testroot = posixpath.join(self.dm.getDeviceRoot(), 'infratest')
        self.dm.removeDir(testroot)
        self.dm.mkDir(testroot)
        self.dm.pushDir(
            os.path.join(self.localsrcdir, 'push1'),
            posixpath.join(testroot, 'push1'))
        
        
        self.dm.mkDir(posixpath.join(testroot, 'push1', 'emptysub'))
        filelist = self.dm.getDirectory(
            posixpath.join(testroot, 'push1'),
            os.path.join(self.localdestdir, 'push1'))
        filelist.sort()
        self.assertEqual(filelist, self.expected_filelist)
        self.assertTrue(os.path.exists(
            os.path.join(self.localdestdir,
                         'push1', 'sub.1', 'sub.2', 'testfile')))
        self.assertTrue(os.path.exists(
            os.path.join(self.localdestdir, 'push1', 'emptysub')))
        filelist = self.dm.getDirectory('/dummy',
            os.path.join(self.localdestdir, '/none'))
        self.assertEqual(filelist, None)
        self.assertFalse(os.path.exists(self.localdestdir + '/none'))
