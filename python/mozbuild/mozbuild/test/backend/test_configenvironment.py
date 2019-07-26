



import os, posixpath
from StringIO import StringIO
import unittest
from mozunit import main, MockedOpen

import mozbuild.backend.configenvironment as ConfigStatus

from mozbuild.util import ReadOnlyDict

import mozpack.path as mozpath


class ConfigEnvironment(ConfigStatus.ConfigEnvironment):
    def __init__(self, *args, **kwargs):
        ConfigStatus.ConfigEnvironment.__init__(self, *args, **kwargs)
        
        if not 'top_srcdir' in self.substs:
            if os.path.isabs(self.topsrcdir):
                top_srcdir = self.topsrcdir.replace(os.sep, '/')
            else:
                top_srcdir = mozpath.relpath(self.topsrcdir, self.topobjdir).replace(os.sep, '/')

            d = dict(self.substs)
            d['top_srcdir'] = top_srcdir
            self.substs = ReadOnlyDict(d)

            d = dict(self.substs_unicode)
            d[u'top_srcdir'] = top_srcdir.decode('utf-8')
            self.substs_unicode = ReadOnlyDict(d)


class TestEnvironment(unittest.TestCase):
    def test_auto_substs(self):
        '''Test the automatically set values of ACDEFINES, ALLDEFINES,
        ALLSUBSTS and ALLEMPTYSUBSTS.
        '''
        env = ConfigEnvironment('.', '.',
                  defines = [ ('foo', 'bar'), ('baz', 'qux 42'),
                              ('abc', "d'e'f"), ('extra', 'foobar') ],
                  non_global_defines = ['extra', 'ignore'],
                  substs = [ ('FOO', 'bar'), ('FOOBAR', ''), ('ABC', 'def'),
                             ('bar', 'baz qux'), ('zzz', '"abc def"'),
                             ('qux', '') ])
        
        
        
        self.assertEqual(env.substs['ACDEFINES'], """-Dfoo='bar' -Dbaz='qux 42' -Dabc='d'\\''e'\\''f'""")
        
        self.assertEqual(env.substs['ALLDEFINES'], '''#define abc d'e'f
#define baz qux 42
#define foo bar''')
        
        
        self.assertEqual(env.substs['ALLSUBSTS'], '''ABC = def
ACDEFINES = -Dfoo='bar' -Dbaz='qux 42' -Dabc='d'\\''e'\\''f'
FOO = bar
bar = baz qux
zzz = "abc def"''')
        
        self.assertEqual(env.substs['ALLEMPTYSUBSTS'], '''FOOBAR =
qux =''')


if __name__ == "__main__":
    main()
