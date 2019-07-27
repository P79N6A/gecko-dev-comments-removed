





import os
import shutil
import tempfile
import unittest
from manifestparser import ManifestParser
from StringIO import StringIO

here = os.path.dirname(os.path.abspath(__file__))

class TestManifestParser(unittest.TestCase):
    """
    Test the manifest parser

    You must have manifestparser installed before running these tests.
    Run ``python manifestparser.py setup develop`` with setuptools installed.
    """

    def test_sanity(self):
        """Ensure basic parser is sane"""

        parser = ManifestParser()
        mozmill_example = os.path.join(here, 'mozmill-example.ini')
        parser.read(mozmill_example)
        tests = parser.tests
        self.assertEqual(len(tests), len(file(mozmill_example).read().strip().splitlines()))

        
        lines = ['[%s]' % test['name'] for test in tests]
        self.assertEqual(lines, file(mozmill_example).read().strip().splitlines())

        
        mozmill_restart_example = os.path.join(here, 'mozmill-restart-example.ini')
        parser.read(mozmill_restart_example)
        restart_tests = parser.get(type='restart')
        self.assertTrue(len(restart_tests) < len(parser.tests))
        self.assertEqual(len(restart_tests), len(parser.get(manifest=mozmill_restart_example)))
        self.assertFalse([test for test in restart_tests
                          if test['manifest'] != os.path.join(here, 'mozmill-restart-example.ini')])
        self.assertEqual(parser.get('name', tags=['foo']),
                         ['restartTests/testExtensionInstallUninstall/test2.js',
                          'restartTests/testExtensionInstallUninstall/test1.js'])
        self.assertEqual(parser.get('name', foo='bar'),
                         ['restartTests/testExtensionInstallUninstall/test2.js'])

    def test_include(self):
        """Illustrate how include works"""

        include_example = os.path.join(here, 'include-example.ini')
        parser = ManifestParser(manifests=(include_example,))

        
        self.assertEqual(parser.get('name'),
                         ['crash-handling', 'fleem', 'flowers'])
        self.assertEqual([(test['name'], os.path.basename(test['manifest'])) for test in parser.tests],
                         [('crash-handling', 'bar.ini'), ('fleem', 'include-example.ini'), ('flowers', 'foo.ini')])


        
        self.assertEqual(len(parser.manifests()), 3)

        
        self.assertEqual(here, parser.rootdir)


        
        
        
        self.assertEqual(parser.get('name', foo='bar'),
                         ['fleem', 'flowers'])
        self.assertEqual(parser.get('name', foo='fleem'),
                         ['crash-handling'])

        
        
        self.assertEqual(parser.get('name', tags=['red']),
                         ['flowers'])

        
        
        
        self.assertEqual(parser.get(name='flowers')[0]['blue'],
                         'ocean')
        self.assertEqual(parser.get(name='flowers')[0]['yellow'],
                         'submarine')

        
        flowers = parser.get(foo='bar')
        self.assertEqual(len(flowers), 2)

        
        self.assertEqual(parser.get('name', inverse=True, tags=['red']),
                         ['crash-handling', 'fleem'])

        
        self.assertEqual([i['name'] for i in parser.missing()], [])

        
        buffer = StringIO()
        parser.write(fp=buffer, global_kwargs={'foo': 'bar'})
        self.assertEqual(buffer.getvalue().strip(),
                         '[DEFAULT]\nfoo = bar\n\n[fleem]\nsubsuite = \n\n[include/flowers]\nblue = ocean\nred = roses\nsubsuite = \nyellow = submarine')

    def test_invalid_path(self):
        """
        Test invalid path should not throw when not strict
        """
        manifest = os.path.join(here, 'include-invalid.ini')
        parser = ManifestParser(manifests=(manifest,), strict=False)

    def test_parent_inheritance(self):
        """
        Test parent manifest variable inheritance
        Specifically tests that inherited variables from parent includes
        properly propagate downstream
        """
        parent_example = os.path.join(here, 'parent', 'level_1', 'level_2',
                                      'level_3', 'level_3.ini')
        parser = ManifestParser(manifests=(parent_example,))

        
        self.assertEqual(parser.get('name'),
                         ['test_3'])
        self.assertEqual([(test['name'], os.path.basename(test['manifest'])) for test in parser.tests],
                         [('test_3', 'level_3.ini')])

        
        self.assertEqual(parser.get('name', x='level_1'),
                         ['test_3'])

        
        buffer = StringIO()
        parser.write(fp=buffer, global_kwargs={'x': 'level_1'})
        self.assertEqual(buffer.getvalue().strip(),
                         '[DEFAULT]\nx = level_1\n\n[test_3]\nsubsuite =')

    def test_parent_defaults(self):
        """
        Test downstream variables should overwrite upstream variables
        """
        parent_example = os.path.join(here, 'parent', 'level_1', 'level_2',
                                      'level_3', 'level_3_default.ini')
        parser = ManifestParser(manifests=(parent_example,))

        
        self.assertEqual(parser.get('name'),
                         ['test_3'])
        self.assertEqual([(test['name'], os.path.basename(test['manifest'])) for test in parser.tests],
                         [('test_3', 'level_3_default.ini')])

        
        self.assertEqual(parser.get('name', x='level_3'),
                         ['test_3'])

        
        buffer = StringIO()
        parser.write(fp=buffer, global_kwargs={'x': 'level_3'})
        self.assertEqual(buffer.getvalue().strip(),
                         '[DEFAULT]\nx = level_3\n\n[test_3]\nsubsuite =')

    def test_server_root(self):
        """
        Test server_root properly expands as an absolute path
        """
        server_example = os.path.join(here, 'parent', 'level_1', 'level_2',
                                      'level_3', 'level_3_server-root.ini')
        parser = ManifestParser(manifests=(server_example,))

        
        self.assertEqual(parser.get('name', **{'other-root': '../root'}),
                                    ['test_3'])

        
        
        self.assertEqual(parser.get('name', **{'server-root': '../root'}), [])

        
        self.assertEqual(parser.get('server-root')[0],
                         os.path.join(here, 'parent', 'root'))

    def test_copy(self):
        """Test our ability to copy a set of manifests"""

        tempdir = tempfile.mkdtemp()
        include_example = os.path.join(here, 'include-example.ini')
        manifest = ManifestParser(manifests=(include_example,))
        manifest.copy(tempdir)
        self.assertEqual(sorted(os.listdir(tempdir)),
                         ['fleem', 'include', 'include-example.ini'])
        self.assertEqual(sorted(os.listdir(os.path.join(tempdir, 'include'))),
                         ['bar.ini', 'crash-handling', 'flowers', 'foo.ini'])
        from_manifest = ManifestParser(manifests=(include_example,))
        to_manifest = os.path.join(tempdir, 'include-example.ini')
        to_manifest = ManifestParser(manifests=(to_manifest,))
        self.assertEqual(to_manifest.get('name'), from_manifest.get('name'))
        shutil.rmtree(tempdir)

    def test_path_override(self):
        """You can override the path in the section too.
        This shows that you can use a relative path"""
        path_example = os.path.join(here, 'path-example.ini')
        manifest = ManifestParser(manifests=(path_example,))
        self.assertEqual(manifest.tests[0]['path'],
                         os.path.join(here, 'fleem'))

    def test_relative_path(self):
        """
        Relative test paths are correctly calculated.
        """
        relative_path = os.path.join(here, 'relative-path.ini')
        manifest = ManifestParser(manifests=(relative_path,))
        self.assertEqual(manifest.tests[0]['path'],
                         os.path.join(os.path.dirname(here), 'fleem'))
        self.assertEqual(manifest.tests[0]['relpath'],
                         os.path.join('..', 'fleem'))
        self.assertEqual(manifest.tests[1]['relpath'],
                         os.path.join('..', 'testsSIBLING', 'example'))

    def test_path_from_fd(self):
        """
        Test paths are left untouched when manifest is a file-like object.
        """
        fp = StringIO("[section]\npath=fleem")
        manifest = ManifestParser(manifests=(fp,))
        self.assertEqual(manifest.tests[0]['path'], 'fleem')
        self.assertEqual(manifest.tests[0]['relpath'], 'fleem')
        self.assertEqual(manifest.tests[0]['manifest'], None)

    def test_comments(self):
        """
        ensure comments work, see
        https://bugzilla.mozilla.org/show_bug.cgi?id=813674
        """
        comment_example = os.path.join(here, 'comment-example.ini')
        manifest = ManifestParser(manifests=(comment_example,))
        self.assertEqual(len(manifest.tests), 8)
        names = [i['name'] for i in manifest.tests]
        self.assertFalse('test_0202_app_launch_apply_update_dirlocked.js' in names)

    def test_verifyDirectory(self):

        directory = os.path.join(here, 'verifyDirectory')

        
        manifest_path = os.path.join(directory, 'verifyDirectory.ini')
        manifest = ManifestParser(manifests=(manifest_path,))
        missing = manifest.verifyDirectory(directory, extensions=('.js',))
        self.assertEqual(missing, (set(), set()))

        
        test_1 = os.path.join(directory, 'test_1.js')
        manifest_path = os.path.join(directory, 'verifyDirectory_incomplete.ini')
        manifest = ManifestParser(manifests=(manifest_path,))
        missing = manifest.verifyDirectory(directory, extensions=('.js',))
        self.assertEqual(missing, (set(), set([test_1])))

        
        missing_test = os.path.join(directory, 'test_notappearinginthisfilm.js')
        manifest_path = os.path.join(directory, 'verifyDirectory_toocomplete.ini')
        manifest = ManifestParser(manifests=(manifest_path,))
        missing = manifest.verifyDirectory(directory, extensions=('.js',))
        self.assertEqual(missing, (set([missing_test]), set()))

    def test_just_defaults(self):
        """Ensure a manifest with just a DEFAULT section exposes that data."""

        parser = ManifestParser()
        manifest = os.path.join(here, 'just-defaults.ini')
        parser.read(manifest)
        self.assertEqual(len(parser.tests), 0)
        self.assertTrue(manifest in parser.manifest_defaults)
        self.assertEquals(parser.manifest_defaults[manifest]['foo'], 'bar')

    def test_manifest_list(self):
        """
        Ensure a manifest with just a DEFAULT section still returns
        itself from the manifests() method.
        """

        parser = ManifestParser()
        manifest = os.path.join(here, 'no-tests.ini')
        parser.read(manifest)
        self.assertEqual(len(parser.tests), 0)
        self.assertTrue(len(parser.manifests()) == 1)

if __name__ == '__main__':
    unittest.main()
