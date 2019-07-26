



from __future__ import unicode_literals

import os
import unittest

from mozpack.manifests import (
    InstallManifest,
)
from mozunit import main

from mozbuild.backend.recursivemake import (
    RecursiveMakeBackend,
    RecursiveMakeTraversal,
)
from mozbuild.frontend.emitter import TreeMetadataEmitter
from mozbuild.frontend.reader import BuildReader

from mozbuild.test.backend.common import BackendTester


class TestRecursiveMakeTraversal(unittest.TestCase):
    def test_traversal(self):
        traversal = RecursiveMakeTraversal()
        traversal.add('', dirs=['A', 'B', 'C'])
        traversal.add('', dirs=['D'])
        traversal.add('A')
        traversal.add('B', dirs=['E', 'F'])
        traversal.add('C', parallel=['G', 'H'])
        traversal.add('D', parallel=['I'], dirs=['K'])
        traversal.add('D', parallel=['J'], dirs=['L'])
        traversal.add('E')
        traversal.add('F')
        traversal.add('G')
        traversal.add('H')
        traversal.add('I', dirs=['M', 'N'])
        traversal.add('J', parallel=['O', 'P'])
        traversal.add('K', parallel=['Q', 'R'])
        traversal.add('L', dirs=['S'])
        traversal.add('M')
        traversal.add('N', dirs=['T'])
        traversal.add('O')
        traversal.add('P', parallel=['U'])
        traversal.add('Q')
        traversal.add('R', dirs=['V'])
        traversal.add('S', dirs=['W'])
        traversal.add('T')
        traversal.add('U')
        traversal.add('V')
        traversal.add('W', dirs=['X'])
        traversal.add('X')

        start, deps = traversal.compute_dependencies()
        self.assertEqual(start, ('X',))
        self.assertEqual(deps, {
            'A': ('',),
            'B': ('A',),
            'C': ('F',),
            'D': ('G', 'H'),
            'E': ('B',),
            'F': ('E',),
            'G': ('C',),
            'H': ('C',),
            'I': ('D',),
            'J': ('D',),
            'K': ('T', 'O', 'U'),
            'L': ('Q', 'V'),
            'M': ('I',),
            'N': ('M',),
            'O': ('J',),
            'P': ('J',),
            'Q': ('K',),
            'R': ('K',),
            'S': ('L',),
            'T': ('N',),
            'U': ('P',),
            'V': ('R',),
            'W': ('S',),
            'X': ('W',),
        })

        self.assertEqual(list(traversal.traverse('')),
                         ['', 'A', 'B', 'E', 'F', 'C', 'G', 'H', 'D', 'I',
                         'M', 'N', 'T', 'J', 'O', 'P', 'U', 'K', 'Q', 'R',
                         'V', 'L', 'S', 'W', 'X'])

        self.assertEqual(list(traversal.traverse('C')),
                         ['C', 'G', 'H'])

    def test_traversal_2(self):
        traversal = RecursiveMakeTraversal()
        traversal.add('', dirs=['A', 'B', 'C'])
        traversal.add('A')
        traversal.add('B', static=['D'], dirs=['E', 'F'])
        traversal.add('C', parallel=['G', 'H'], dirs=['I'])
        
        traversal.add('E')
        traversal.add('F')
        traversal.add('G')
        traversal.add('H')
        traversal.add('I')

        start, deps = traversal.compute_dependencies()
        self.assertEqual(start, ('I',))
        self.assertEqual(deps, {
            'A': ('',),
            'B': ('A',),
            'C': ('F',),
            'D': ('B',),
            'E': ('D',),
            'F': ('E',),
            'G': ('C',),
            'H': ('C',),
            'I': ('G', 'H'),
        })

    def test_traversal_filter(self):
        traversal = RecursiveMakeTraversal()
        traversal.add('', dirs=['A', 'B', 'C'])
        traversal.add('A')
        traversal.add('B', static=['D'], dirs=['E', 'F'])
        traversal.add('C', parallel=['G', 'H'], dirs=['I'])
        traversal.add('D')
        traversal.add('E')
        traversal.add('F')
        traversal.add('G')
        traversal.add('H')
        traversal.add('I')

        def filter(current, subdirs):
            if current == 'B':
                current = None
            return current, subdirs.parallel, subdirs.dirs

        start, deps = traversal.compute_dependencies(filter)
        self.assertEqual(start, ('I',))
        self.assertEqual(deps, {
            'A': ('',),
            'C': ('F',),
            'E': ('A',),
            'F': ('E',),
            'G': ('C',),
            'H': ('C',),
            'I': ('G', 'H'),
        })

class TestRecursiveMakeBackend(BackendTester):
    def test_basic(self):
        """Ensure the RecursiveMakeBackend works without error."""
        env = self._consume('stub0', RecursiveMakeBackend)
        self.assertTrue(os.path.exists(os.path.join(env.topobjdir,
            'backend.RecursiveMakeBackend.built')))
        self.assertTrue(os.path.exists(os.path.join(env.topobjdir,
            'backend.RecursiveMakeBackend.built.pp')))

    def test_output_files(self):
        """Ensure proper files are generated."""
        env = self._consume('stub0', RecursiveMakeBackend)

        expected = ['', 'dir1', 'dir2']

        for d in expected:
            out_makefile = os.path.join(env.topobjdir, d, 'Makefile')
            out_backend = os.path.join(env.topobjdir, d, 'backend.mk')

            self.assertTrue(os.path.exists(out_makefile))
            self.assertTrue(os.path.exists(out_backend))

    def test_makefile_conversion(self):
        """Ensure Makefile.in is converted properly."""
        env = self._consume('stub0', RecursiveMakeBackend)

        p = os.path.join(env.topobjdir, 'Makefile')

        lines = [l.strip() for l in open(p, 'rt').readlines()[1:] if not l.startswith('#')]
        self.assertEqual(lines, [
            'DEPTH := .',
            'topsrcdir := %s' % env.topsrcdir,
            'srcdir := %s' % env.topsrcdir,
            'VPATH := %s' % env.topsrcdir,
            'relativesrcdir := .',
            'include $(DEPTH)/config/autoconf.mk',
            '',
            'FOO := foo',
            '',
            'include $(topsrcdir)/config/recurse.mk',
        ])

    def test_missing_makefile_in(self):
        """Ensure missing Makefile.in results in Makefile creation."""
        env = self._consume('stub0', RecursiveMakeBackend)

        p = os.path.join(env.topobjdir, 'dir2', 'Makefile')
        self.assertTrue(os.path.exists(p))

        lines = [l.strip() for l in open(p, 'rt').readlines()]
        self.assertEqual(len(lines), 9)

        self.assertTrue(lines[0].startswith('# THIS FILE WAS AUTOMATICALLY'))

    def test_backend_mk(self):
        """Ensure backend.mk file is written out properly."""
        env = self._consume('stub0', RecursiveMakeBackend)

        p = os.path.join(env.topobjdir, 'backend.mk')

        lines = [l.strip() for l in open(p, 'rt').readlines()[2:]]
        self.assertEqual(lines, [
            'MOZBUILD_DERIVED := 1',
            'NO_MAKEFILE_RULE := 1',
            'NO_SUBMAKEFILES_RULE := 1',
            'DIRS := dir1',
            'PARALLEL_DIRS := dir2',
            'TEST_DIRS := dir3',
        ])

    def test_mtime_no_change(self):
        """Ensure mtime is not updated if file content does not change."""

        env = self._consume('stub0', RecursiveMakeBackend)

        makefile_path = os.path.join(env.topobjdir, 'Makefile')
        backend_path = os.path.join(env.topobjdir, 'backend.mk')
        makefile_mtime = os.path.getmtime(makefile_path)
        backend_mtime = os.path.getmtime(backend_path)

        reader = BuildReader(env)
        emitter = TreeMetadataEmitter(env)
        backend = RecursiveMakeBackend(env)
        backend.consume(emitter.emit(reader.read_topsrcdir()))

        self.assertEqual(os.path.getmtime(makefile_path), makefile_mtime)
        self.assertEqual(os.path.getmtime(backend_path), backend_mtime)

    def test_external_make_dirs(self):
        """Ensure we have make recursion into external make directories."""
        env = self._consume('external_make_dirs', RecursiveMakeBackend)

        backend_path = os.path.join(env.topobjdir, 'backend.mk')
        lines = [l.strip() for l in open(backend_path, 'rt').readlines()[2:]]
        self.assertEqual(lines, [
            'MOZBUILD_DERIVED := 1',
            'NO_MAKEFILE_RULE := 1',
            'NO_SUBMAKEFILES_RULE := 1',
            'DIRS := dir',
            'PARALLEL_DIRS := p_dir',
            'DIRS += external',
            'PARALLEL_DIRS += p_external',
        ])

    def test_substitute_config_files(self):
        """Ensure substituted config files are produced."""
        env = self._consume('substitute_config_files', RecursiveMakeBackend)

        p = os.path.join(env.topobjdir, 'foo')
        self.assertTrue(os.path.exists(p))
        lines = [l.strip() for l in open(p, 'rt').readlines()]
        self.assertEqual(lines, [
            'TEST = foo',
        ])

    def test_variable_passthru(self):
        """Ensure variable passthru is written out correctly."""
        env = self._consume('variable_passthru', RecursiveMakeBackend)

        backend_path = os.path.join(env.topobjdir, 'backend.mk')
        lines = [l.strip() for l in open(backend_path, 'rt').readlines()[2:]]

        expected = {
            'ASFILES': [
                'ASFILES += bar.s',
                'ASFILES += foo.asm',
            ],
            'CMMSRCS': [
                'CMMSRCS += bar.mm',
                'CMMSRCS += foo.mm',
            ],
            'CPP_UNIT_TESTS': [
                'CPP_UNIT_TESTS += foo.cpp',
            ],
            'CSRCS': [
                'CSRCS += bar.c',
                'CSRCS += foo.c',
            ],
            'EXTRA_COMPONENTS': [
                'EXTRA_COMPONENTS += bar.js',
                'EXTRA_COMPONENTS += foo.js',
            ],
            'EXTRA_PP_COMPONENTS': [
                'EXTRA_PP_COMPONENTS += bar.pp.js',
                'EXTRA_PP_COMPONENTS += foo.pp.js',
            ],
            'EXTRA_JS_MODULES': [
                'EXTRA_JS_MODULES += bar.jsm',
                'EXTRA_JS_MODULES += foo.jsm',
            ],
            'EXTRA_PP_JS_MODULES': [
                'EXTRA_PP_JS_MODULES += bar.pp.jsm',
                'EXTRA_PP_JS_MODULES += foo.pp.jsm',
            ],
            'FAIL_ON_WARNINGS': [
                'FAIL_ON_WARNINGS := 1',
            ],
            'GTEST_CMMSRCS': [
                'GTEST_CMMSRCS += test1.mm',
                'GTEST_CMMSRCS += test2.mm',
            ],
            'GTEST_CPPSRCS': [
                'GTEST_CPPSRCS += test1.cpp',
                'GTEST_CPPSRCS += test2.cpp',
            ],
            'GTEST_CSRCS': [
                'GTEST_CSRCS += test1.c',
                'GTEST_CSRCS += test2.c',
            ],
            'HOST_CPPSRCS': [
                'HOST_CPPSRCS += bar.cpp',
                'HOST_CPPSRCS += foo.cpp',
            ],
            'HOST_CSRCS': [
                'HOST_CSRCS += bar.c',
                'HOST_CSRCS += foo.c',
            ],
            'HOST_LIBRARY_NAME': [
                'HOST_LIBRARY_NAME := host_bar',
            ],
            'LIBRARY_NAME': [
                'LIBRARY_NAME := lib_name',
            ],
            'LIBXUL_LIBRARY': [
                'LIBXUL_LIBRARY := 1',
            ],
            'MSVC_ENABLE_PGO': [
                'MSVC_ENABLE_PGO := 1',
            ],
            'OS_LIBS': [
                'OS_LIBS += foo.so',
                'OS_LIBS += -l123',
                'OS_LIBS += bar.a',
            ],
            'SDK_LIBRARY': [
                'SDK_LIBRARY += bar.sdk',
                'SDK_LIBRARY += foo.sdk',
            ],
            'SHARED_LIBRARY_LIBS': [
                'SHARED_LIBRARY_LIBS += bar.sll',
                'SHARED_LIBRARY_LIBS += foo.sll',
            ],
            'SIMPLE_PROGRAMS': [
                'SIMPLE_PROGRAMS += bar.x',
                'SIMPLE_PROGRAMS += foo.x',
            ],
            'SSRCS': [
                'SSRCS += bar.S',
                'SSRCS += foo.S',
            ],
        }

        for var, val in expected.items():
            
            found = [str for str in lines if str.startswith(var)]
            self.assertEqual(found, val)

    def test_exports(self):
        """Ensure EXPORTS is handled properly."""
        env = self._consume('exports', RecursiveMakeBackend)

        
        m = InstallManifest(path=os.path.join(env.topobjdir,
            '_build_manifests', 'install', 'dist_include'))
        self.assertEqual(len(m), 7)
        self.assertIn('foo.h', m)
        self.assertIn('mozilla/mozilla1.h', m)
        self.assertIn('mozilla/dom/dom2.h', m)

    def test_xpcshell_manifests(self):
        """Ensure XPCSHELL_TESTS_MANIFESTS is written out correctly."""
        env = self._consume('xpcshell_manifests', RecursiveMakeBackend)

        backend_path = os.path.join(env.topobjdir, 'backend.mk')
        lines = [l.strip() for l in open(backend_path, 'rt').readlines()[2:]]

        
        var = 'XPCSHELL_TESTS'
        xpclines = sorted([val for val in lines if val.startswith(var)])

        
        expected = ('aa', 'bb', 'cc', 'dd', 'valid_val')
        self.assertEqual(xpclines, ["XPCSHELL_TESTS += %s" % val for val in expected])

    def test_xpidl_generation(self):
        """Ensure xpidl files and directories are written out."""
        env = self._consume('xpidl', RecursiveMakeBackend)

        
        install_dir = os.path.join(env.topobjdir, '_build_manifests',
            'install')
        self.assertTrue(os.path.isfile(os.path.join(install_dir, 'dist_idl')))
        self.assertTrue(os.path.isfile(os.path.join(install_dir, 'xpidl')))

        m = InstallManifest(path=os.path.join(install_dir, 'dist_idl'))
        self.assertEqual(len(m), 2)
        self.assertIn('bar.idl', m)
        self.assertIn('foo.idl', m)

        m = InstallManifest(path=os.path.join(install_dir, 'xpidl'))
        self.assertIn('.deps/my_module.pp', m)
        self.assertIn('xpt/my_module.xpt', m)

        m = InstallManifest(path=os.path.join(install_dir, 'dist_include'))
        self.assertIn('foo.h', m)

        p = os.path.join(env.topobjdir, 'config/makefiles/xpidl')
        self.assertTrue(os.path.isdir(p))

        self.assertTrue(os.path.isfile(os.path.join(p, 'Makefile')))

    def test_xpcshell_master_manifest(self):
        """Ensure that the master xpcshell manifest is written out correctly."""
        env = self._consume('xpcshell_manifests', RecursiveMakeBackend)

        manifest_path = os.path.join(env.topobjdir,
            'testing', 'xpcshell', 'xpcshell.ini')
        lines = [l.strip() for l in open(manifest_path, 'rt').readlines()]
        expected = ('aa', 'bb', 'cc', 'dd', 'valid_val')
        self.assertEqual(lines, [
            '; THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT MODIFY BY HAND.',
            ''] + ['[include:%s/xpcshell.ini]' % x for x in expected])

    def test_old_install_manifest_deleted(self):
        
        
        env = self._get_environment('stub0')
        purge_dir = os.path.join(env.topobjdir, '_build_manifests', 'install')
        manifest_path = os.path.join(purge_dir, 'old_manifest')
        os.makedirs(purge_dir)
        m = InstallManifest()
        m.write(path=manifest_path)

        self.assertTrue(os.path.exists(manifest_path))
        self._consume('stub0', RecursiveMakeBackend, env)
        self.assertFalse(os.path.exists(manifest_path))

    def test_install_manifests_written(self):
        env, objs = self._emit('stub0')
        backend = RecursiveMakeBackend(env)

        m = InstallManifest()
        backend._install_manifests['testing'] = m
        m.add_symlink(__file__, 'self')
        backend.consume(objs)

        man_dir = os.path.join(env.topobjdir, '_build_manifests', 'install')
        self.assertTrue(os.path.isdir(man_dir))

        expected = ['testing']
        for e in expected:
            full = os.path.join(man_dir, e)
            self.assertTrue(os.path.exists(full))

            m2 = InstallManifest(path=full)
            self.assertEqual(m, m2)

    def test_ipdl_sources(self):
        """Test that IPDL_SOURCES are written to ipdlsrcs.mk correctly."""
        env = self._consume('ipdl_sources', RecursiveMakeBackend)

        manifest_path = os.path.join(env.topobjdir,
            'ipc', 'ipdl', 'ipdlsrcs.mk')
        lines = [l.strip() for l in open(manifest_path, 'rt').readlines()]

        
        topsrcdir = env.topsrcdir.replace(os.sep, '/')

        expected = [
            "ALL_IPDLSRCS += %s/bar/bar.ipdl" % topsrcdir,
            "CPPSRCS += bar.cpp",
            "CPPSRCS += barChild.cpp",
            "CPPSRCS += barParent.cpp",
            "ALL_IPDLSRCS += %s/bar/bar2.ipdlh" % topsrcdir,
            "CPPSRCS += bar2.cpp",
            "ALL_IPDLSRCS += %s/foo/foo.ipdl" % topsrcdir,
            "CPPSRCS += foo.cpp",
            "CPPSRCS += fooChild.cpp",
            "CPPSRCS += fooParent.cpp",
            "ALL_IPDLSRCS += %s/foo/foo2.ipdlh" % topsrcdir,
            "CPPSRCS += foo2.cpp",
            "IPDLDIRS := %s/bar %s/foo" % (topsrcdir, topsrcdir),
        ]
        self.assertEqual(lines, expected)

    def test_defines(self):
        """Test that DEFINES are written to backend.mk correctly."""
        env = self._consume('defines', RecursiveMakeBackend)

        backend_path = os.path.join(env.topobjdir, 'backend.mk')
        lines = [l.strip() for l in open(backend_path, 'rt').readlines()[2:]]

        var = 'DEFINES'
        defines = [val for val in lines if val.startswith(var)]

        expected = ['DEFINES += -DFOO -DBAZ=\'"abcd"\' -DBAR=7 -DVALUE=\'xyz\'']
        self.assertEqual(defines, expected)

    def test_local_includes(self):
        """Test that LOCAL_INCLUDES are written to backend.mk correctly."""
        env = self._consume('local_includes', RecursiveMakeBackend)

        backend_path = os.path.join(env.topobjdir, 'backend.mk')
        lines = [l.strip() for l in open(backend_path, 'rt').readlines()[2:]]

        expected = [
            'LOCAL_INCLUDES += -I$(topsrcdir)/bar/baz',
            'LOCAL_INCLUDES += -I$(srcdir)/foo',
        ]

        found = [str for str in lines if str.startswith('LOCAL_INCLUDES')]
        self.assertEqual(found, expected)

if __name__ == '__main__':
    main()
