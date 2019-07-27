



from __future__ import unicode_literals

import unittest

from mozunit import main

from mozbuild.frontend.sandbox import (
    GlobalNamespace,
    LocalNamespace,
)

from mozbuild.frontend.sandbox_symbols import VARIABLES


class TestGlobalNamespace(unittest.TestCase):
    def test_builtins(self):
        ns = GlobalNamespace()

        self.assertIn('__builtins__', ns)
        self.assertEqual(ns['__builtins__']['True'], True)

    def test_key_rejection(self):
        
        ns = GlobalNamespace(allowed_variables=VARIABLES)

        with self.assertRaises(KeyError) as ke:
            ns['foo'] = True

        e = ke.exception.args
        self.assertEqual(e[0], 'global_ns')
        self.assertEqual(e[1], 'set_unknown')
        self.assertEqual(e[2], 'foo')
        self.assertTrue(e[3])

        
        with self.assertRaises(KeyError) as ke:
            ns['FOO'] = True

        e = ke.exception.args
        self.assertEqual(e[0], 'global_ns')
        self.assertEqual(e[1], 'set_unknown')
        self.assertEqual(e[2], 'FOO')
        self.assertTrue(e[3])

    def test_allowed_set(self):
        self.assertIn('DIRS', VARIABLES)

        ns = GlobalNamespace(allowed_variables=VARIABLES)

        ns['DIRS'] = ['foo']
        self.assertEqual(ns['DIRS'], ['foo'])

    def test_value_checking(self):
        ns = GlobalNamespace(allowed_variables=VARIABLES)

        
        with self.assertRaises(ValueError) as ve:
            ns['DIRS'] = True

        e = ve.exception.args
        self.assertEqual(e[0], 'global_ns')
        self.assertEqual(e[1], 'set_type')
        self.assertEqual(e[2], 'DIRS')
        self.assertTrue(e[3])
        self.assertEqual(e[4], list)

    def test_allow_all_writes(self):
        ns = GlobalNamespace(allowed_variables=VARIABLES)

        with ns.allow_all_writes() as d:
            d['foo'] = True
            self.assertTrue(d['foo'])

        with self.assertRaises(KeyError) as ke:
            ns['bar'] = False

        self.assertEqual(ke.exception.args[1], 'set_unknown')

        ns['DIRS'] = []
        with self.assertRaises(KeyError) as ke:
            ns['DIRS'] = []

        e = ke.exception.args
        self.assertEqual(e[0], 'global_ns')
        self.assertEqual(e[1], 'reassign')
        self.assertEqual(e[2], 'DIRS')

        with ns.allow_all_writes() as d:
            d['DIST_SUBDIR'] = 'foo'

        self.assertEqual(ns['DIST_SUBDIR'], 'foo')
        ns['DIST_SUBDIR'] = 'bar'
        self.assertEqual(ns['DIST_SUBDIR'], 'bar')
        with self.assertRaises(KeyError) as ke:
            ns['DIST_SUBDIR'] = 'baz'

        e = ke.exception.args
        self.assertEqual(e[0], 'global_ns')
        self.assertEqual(e[1], 'reassign')
        self.assertEqual(e[2], 'DIST_SUBDIR')

        self.assertTrue(d['foo'])

    def test_key_checking(self):
        
        
        g = GlobalNamespace(allowed_variables=VARIABLES)

        self.assertFalse('DIRS' in g)
        self.assertFalse('DIRS' in g)


class TestLocalNamespace(unittest.TestCase):
    def test_locals(self):
        g = GlobalNamespace(allowed_variables=VARIABLES)
        l = LocalNamespace(g)

        l['foo'] = ['foo']
        self.assertEqual(l['foo'], ['foo'])

        l['foo'] += ['bar']
        self.assertEqual(l['foo'], ['foo', 'bar'])

    def test_global_proxy_reads(self):
        g = GlobalNamespace(allowed_variables=VARIABLES)
        g['DIRS'] = ['foo']

        l = LocalNamespace(g)

        self.assertEqual(l['DIRS'], g['DIRS'])

        
        with self.assertRaises(KeyError) as ke:
            v = l['FOO']

        e = ke.exception
        self.assertEqual(e.args[0], 'global_ns')
        self.assertEqual(e.args[1], 'get_unknown')

    def test_global_proxy_writes(self):
        g = GlobalNamespace(allowed_variables=VARIABLES)
        l = LocalNamespace(g)

        l['DIRS'] = ['foo']

        self.assertEqual(l['DIRS'], ['foo'])
        self.assertEqual(g['DIRS'], ['foo'])


if __name__ == '__main__':
    main()
