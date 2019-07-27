



from __future__ import unicode_literals

import unittest

from mozunit import main

from mozbuild.frontend.context import (
    Context,
    VARIABLES,
)


class TestContext(unittest.TestCase):
    def test_key_rejection(self):
        
        ns = Context(allowed_variables=VARIABLES)

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

        ns = Context(allowed_variables=VARIABLES)

        ns['DIRS'] = ['foo']
        self.assertEqual(ns['DIRS'], ['foo'])

    def test_value_checking(self):
        ns = Context(allowed_variables=VARIABLES)

        
        with self.assertRaises(ValueError) as ve:
            ns['DIRS'] = True

        e = ve.exception.args
        self.assertEqual(e[0], 'global_ns')
        self.assertEqual(e[1], 'set_type')
        self.assertEqual(e[2], 'DIRS')
        self.assertTrue(e[3])
        self.assertEqual(e[4], list)

    def test_key_checking(self):
        
        
        g = Context(allowed_variables=VARIABLES)

        self.assertFalse('DIRS' in g)
        self.assertFalse('DIRS' in g)


if __name__ == '__main__':
    main()
