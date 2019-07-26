



from __future__ import unicode_literals

import os
import shutil
import unittest

from mozunit import main

from mozbuild.frontend.reader import (
    MozbuildSandbox,
    SandboxCalledError,
)

from mozbuild.frontend.sandbox import (
    SandboxExecutionError,
    SandboxLoadError,
)

from mozbuild.frontend.sandbox_symbols import (
    FUNCTIONS,
    SPECIAL_VARIABLES,
    VARIABLES,
)

from mozbuild.test.common import MockConfig


test_data_path = os.path.abspath(os.path.dirname(__file__))
test_data_path = os.path.join(test_data_path, 'data')


class TestSandbox(unittest.TestCase):
    def sandbox(self, relpath='moz.build', data_path=None):
        config = None

        if data_path is not None:
            config = MockConfig(os.path.join(test_data_path, data_path))
        else:
            config = MockConfig()

        return MozbuildSandbox(config, config.child_path(relpath))

    def test_default_state(self):
        sandbox = self.sandbox()
        config = sandbox.config

        self.assertEqual(sandbox['TOPSRCDIR'], config.topsrcdir)
        self.assertEqual(sandbox['TOPOBJDIR'],
            os.path.abspath(config.topobjdir))
        self.assertEqual(sandbox['RELATIVEDIR'], '')
        self.assertEqual(sandbox['SRCDIR'], config.topsrcdir)
        self.assertEqual(sandbox['OBJDIR'],
            os.path.abspath(config.topobjdir).replace(os.sep, '/'))

    def test_symbol_presence(self):
        
        
        sandbox = self.sandbox()

        all_symbols = set()
        all_symbols |= set(FUNCTIONS.keys())
        all_symbols |= set(SPECIAL_VARIABLES.keys())

        for symbol in sandbox:
            self.assertIn(symbol, all_symbols)
            all_symbols.remove(symbol)

        self.assertEqual(len(all_symbols), 0)

    def test_path_calculation(self):
        sandbox = self.sandbox('foo/bar/moz.build')
        config = sandbox.config

        self.assertEqual(sandbox['RELATIVEDIR'], 'foo/bar')
        self.assertEqual(sandbox['SRCDIR'], '/'.join([config.topsrcdir,
            'foo/bar']))
        self.assertEqual(sandbox['OBJDIR'],
            os.path.abspath('/'.join([config.topobjdir, 'foo/bar'])).replace(os.sep, '/'))

    def test_config_access(self):
        sandbox = self.sandbox()
        config = sandbox.config

        self.assertIn('CONFIG', sandbox)
        self.assertEqual(sandbox['CONFIG']['MOZ_TRUE'], '1')
        self.assertEqual(sandbox['CONFIG']['MOZ_FOO'], config.substs['MOZ_FOO'])

        
        self.assertNotIn('MISSING', sandbox['CONFIG'])
        self.assertIsNone(sandbox['CONFIG']['MISSING'])

        
        with self.assertRaises(Exception):
            sandbox['CONFIG']['FOO'] = ''

    def test_dict_interface(self):
        sandbox = self.sandbox()
        config = sandbox.config

        self.assertFalse('foo' in sandbox)
        self.assertFalse('FOO' in sandbox)

        self.assertTrue(sandbox.get('foo', True))
        self.assertEqual(sandbox.get('TOPSRCDIR'), config.topsrcdir)
        self.assertGreater(len(sandbox), 6)

        for key in sandbox:
            continue

        for key in sandbox.iterkeys():
            continue

    def test_exec_source_success(self):
        sandbox = self.sandbox()

        sandbox.exec_source('foo = True', 'foo.py')

        self.assertNotIn('foo', sandbox)
        self.assertEqual(sandbox.main_path, 'foo.py')
        self.assertEqual(sandbox.all_paths, set(['foo.py']))

    def test_exec_compile_error(self):
        sandbox = self.sandbox()

        with self.assertRaises(SandboxExecutionError) as se:
            sandbox.exec_source('2f23;k;asfj', 'foo.py')

        self.assertEqual(se.exception.file_stack, ['foo.py'])
        self.assertIsInstance(se.exception.exc_value, SyntaxError)
        self.assertEqual(sandbox.main_path, 'foo.py')

    def test_exec_import_denied(self):
        sandbox = self.sandbox()

        with self.assertRaises(SandboxExecutionError) as se:
            sandbox.exec_source('import sys', 'import.py')

        self.assertIsInstance(se.exception, SandboxExecutionError)
        self.assertEqual(se.exception.exc_type, ImportError)

    def test_exec_source_multiple(self):
        sandbox = self.sandbox()

        sandbox.exec_source('DIRS = ["foo"]', 'foo.py')
        sandbox.exec_source('DIRS = ["bar"]', 'foo.py')

        self.assertEqual(sandbox['DIRS'], ['bar'])

    def test_exec_source_illegal_key_set(self):
        sandbox = self.sandbox()

        with self.assertRaises(SandboxExecutionError) as se:
            sandbox.exec_source('ILLEGAL = True', 'foo.py')

        e = se.exception
        self.assertIsInstance(e.exc_value, KeyError)

        e = se.exception.exc_value
        self.assertEqual(e.args[0], 'global_ns')
        self.assertEqual(e.args[1], 'set_unknown')

    def test_add_tier_dir_regular_str(self):
        sandbox = self.sandbox()

        sandbox.exec_source('add_tier_dir("t1", "foo")', 'foo.py')

        self.assertEqual(sandbox['TIERS']['t1'],
            {'regular': ['foo'], 'static': []})

    def test_add_tier_dir_regular_list(self):
        sandbox = self.sandbox()

        sandbox.exec_source('add_tier_dir("t1", ["foo", "bar"])', 'foo.py')

        self.assertEqual(sandbox['TIERS']['t1'],
            {'regular': ['foo', 'bar'], 'static': []})

    def test_add_tier_dir_static(self):
        sandbox = self.sandbox()

        sandbox.exec_source('add_tier_dir("t1", "foo", static=True)', 'foo.py')

        self.assertEqual(sandbox['TIERS']['t1'],
            {'regular': [], 'static': ['foo']})

    def test_tier_order(self):
        sandbox = self.sandbox()

        source = '''
add_tier_dir('t1', 'foo')
add_tier_dir('t1', 'bar')
add_tier_dir('t2', 'baz', static=True)
add_tier_dir('t3', 'biz')
add_tier_dir('t1', 'bat', static=True)
'''

        sandbox.exec_source(source, 'foo.py')

        self.assertEqual([k for k in sandbox['TIERS'].keys()], ['t1', 't2', 't3'])

    def test_tier_multiple_registration(self):
        sandbox = self.sandbox()

        sandbox.exec_source('add_tier_dir("t1", "foo")', 'foo.py')

        with self.assertRaises(SandboxExecutionError):
            sandbox.exec_source('add_tier_dir("t1", "foo")', 'foo.py')

    def test_include_basic(self):
        sandbox = self.sandbox(data_path='include-basic')

        sandbox.exec_file('moz.build')

        self.assertEqual(sandbox['DIRS'], ['foo', 'bar'])
        self.assertEqual(sandbox.main_path,
            os.path.join(sandbox['TOPSRCDIR'], 'moz.build'))
        self.assertEqual(len(sandbox.all_paths), 2)

    def test_include_outside_topsrcdir(self):
        sandbox = self.sandbox(data_path='include-outside-topsrcdir')

        with self.assertRaises(SandboxLoadError) as se:
            sandbox.exec_file('relative.build')

        expected = os.path.join(test_data_path, 'moz.build')
        self.assertEqual(se.exception.illegal_path, expected)

    def test_include_error_stack(self):
        
        sandbox = self.sandbox(data_path='include-file-stack')

        with self.assertRaises(SandboxExecutionError) as se:
            sandbox.exec_file('moz.build')

        e = se.exception
        self.assertIsInstance(e.exc_value, KeyError)

        args = e.exc_value.args
        self.assertEqual(args[0], 'global_ns')
        self.assertEqual(args[1], 'set_unknown')
        self.assertEqual(args[2], 'ILLEGAL')

        expected_stack = [os.path.join(sandbox.config.topsrcdir, p) for p in [
            'moz.build', 'included-1.build', 'included-2.build']]

        self.assertEqual(e.file_stack, expected_stack)

    def test_include_missing(self):
        sandbox = self.sandbox(data_path='include-missing')

        with self.assertRaises(SandboxLoadError) as sle:
            sandbox.exec_file('moz.build')

        self.assertIsNotNone(sle.exception.read_error)

    def test_include_relative_from_child_dir(self):
        
        
        sandbox = self.sandbox(data_path='include-relative-from-child')
        sandbox.exec_file('child/child.build')
        self.assertEqual(sandbox['DIRS'], ['foo'])

        sandbox = self.sandbox(data_path='include-relative-from-child')
        sandbox.exec_file('child/child2.build')
        self.assertEqual(sandbox['DIRS'], ['foo'])

    def test_include_topsrcdir_relative(self):
        

        sandbox = self.sandbox(data_path='include-topsrcdir-relative')
        sandbox.exec_file('moz.build')

        self.assertEqual(sandbox['DIRS'], ['foo'])

    def test_external_make_dirs(self):
        sandbox = self.sandbox()
        sandbox.exec_source('EXTERNAL_MAKE_DIRS += ["foo"]', 'test.py')
        sandbox.exec_source('PARALLEL_EXTERNAL_MAKE_DIRS += ["bar"]', 'test.py')

        self.assertEqual(sandbox['EXTERNAL_MAKE_DIRS'], ['foo'])
        self.assertEqual(sandbox['PARALLEL_EXTERNAL_MAKE_DIRS'], ['bar'])

    def test_error(self):
        sandbox = self.sandbox()

        with self.assertRaises(SandboxCalledError) as sce:
            sandbox.exec_source('error("This is an error.")', 'test.py')

        e = sce.exception
        self.assertEqual(e.message, 'This is an error.')


if __name__ == '__main__':
    main()
