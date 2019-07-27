





import json
import mock
import os
import shutil
import sys
import tempfile
import unittest
import mozinfo

class TestMozinfo(unittest.TestCase):
    def setUp(self):
        reload(mozinfo)
        self.tempdir = os.path.abspath(tempfile.mkdtemp())

        
        
        
        sys.modules['mozbuild'] = None

    def tearDown(self):
        shutil.rmtree(self.tempdir)
        del sys.modules['mozbuild']

    def test_basic(self):
        """Test that mozinfo has a few attributes."""
        self.assertNotEqual(mozinfo.os, None)
        
        self.assertTrue(getattr(mozinfo, "is" + mozinfo.os[0].upper() + mozinfo.os[1:]))

    def test_update(self):
        """Test that mozinfo.update works."""
        mozinfo.update({"foo": 123})
        self.assertEqual(mozinfo.info["foo"], 123)

    def test_update_file(self):
        """Test that mozinfo.update can load a JSON file."""
        j = os.path.join(self.tempdir, "mozinfo.json")
        with open(j, "w") as f:
            f.write(json.dumps({"foo": "xyz"}))
        mozinfo.update(j)
        self.assertEqual(mozinfo.info["foo"], "xyz")

    def test_update_file_invalid_json(self):
        """Test that mozinfo.update handles invalid JSON correctly"""
        j = os.path.join(self.tempdir,'test.json')
        with open(j, 'w') as f:
            f.write('invalid{"json":')
        self.assertRaises(ValueError,mozinfo.update,[j])

    def test_find_and_update_file(self):
        """Test that mozinfo.find_and_update_from_json can
        find mozinfo.json in a directory passed to it."""
        j = os.path.join(self.tempdir, "mozinfo.json")
        with open(j, "w") as f:
            f.write(json.dumps({"foo": "abcdefg"}))
        self.assertEqual(mozinfo.find_and_update_from_json(self.tempdir), j)
        self.assertEqual(mozinfo.info["foo"], "abcdefg")

    def test_find_and_update_file_invalid_json(self):
        """Test that mozinfo.find_and_update_from_json can
        handle invalid JSON"""
        j = os.path.join(self.tempdir, "mozinfo.json")
        with open(j, 'w') as f:
            f.write('invalid{"json":')
        self.assertRaises(ValueError, mozinfo.find_and_update_from_json, self.tempdir)


    def test_find_and_update_file_mozbuild(self):
        """Test that mozinfo.find_and_update_from_json can
        find mozinfo.json using the mozbuild module."""
        j = os.path.join(self.tempdir, "mozinfo.json")
        with open(j, "w") as f:
            f.write(json.dumps({"foo": "123456"}))
        m = mock.MagicMock()
        
        m.MozbuildObject.from_environment.return_value.topobjdir = self.tempdir
        with mock.patch.dict(sys.modules, {"mozbuild": m, "mozbuild.base": m}):
            self.assertEqual(mozinfo.find_and_update_from_json(), j)
        self.assertEqual(mozinfo.info["foo"], "123456")


class TestStringVersion(unittest.TestCase):
    def test_os_version_is_a_StringVersion(self):
        self.assertIsInstance(mozinfo.os_version, mozinfo.StringVersion)

    def test_compare_to_string(self):
        version = mozinfo.StringVersion('10.10')

        self.assertGreater(version, '10.2')
        self.assertGreater('11', version)
        self.assertGreaterEqual(version, '10.10')
        self.assertGreaterEqual('10.11', version)
        self.assertEqual(version, '10.10')
        self.assertEqual('10.10', version)
        self.assertNotEqual(version, '10.2')
        self.assertNotEqual('11', version)
        self.assertLess(version, '11.8.5')
        self.assertLess('10.2', version)
        self.assertLessEqual(version, '11')
        self.assertLessEqual('10.10', version)

    def test_to_string(self):
        self.assertEqual('10.10', str(mozinfo.StringVersion('10.10')))


if __name__ == '__main__':
    unittest.main()
