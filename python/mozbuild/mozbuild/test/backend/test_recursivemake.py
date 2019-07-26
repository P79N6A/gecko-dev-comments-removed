



from __future__ import unicode_literals

import os

from mozunit import main

from mozbuild.backend.configenvironment import ConfigEnvironment
from mozbuild.backend.recursivemake import RecursiveMakeBackend
from mozbuild.frontend.emitter import TreeMetadataEmitter
from mozbuild.frontend.reader import BuildReader

from mozbuild.test.backend.common import BackendTester


class TestRecursiveMakeBackend(BackendTester):
    def test_basic(self):
        """Ensure the RecursiveMakeBackend works without error."""
        env = self._consume('stub0', RecursiveMakeBackend)
        self.assertTrue(os.path.exists(os.path.join(env.topobjdir,
            'backend.RecursiveMakeBackend.built')))

    def test_output_files(self):
        """Ensure proper files are generated."""
        env = self._consume('stub0', RecursiveMakeBackend)

        expected = ['', 'dir1', 'dir2']

        for d in expected:
            in_path = os.path.join(env.topsrcdir, d, 'Makefile.in')
            out_makefile = os.path.join(env.topobjdir, d, 'Makefile')
            out_backend = os.path.join(env.topobjdir, d, 'backend.mk')

            self.assertTrue(os.path.exists(in_path))
            self.assertTrue(os.path.exists(out_makefile))
            self.assertTrue(os.path.exists(out_backend))

    def test_makefile_conversion(self):
        """Ensure Makefile.in is converted properly."""
        env = self._consume('stub0', RecursiveMakeBackend)

        p = os.path.join(env.topobjdir, 'Makefile')

        lines = [l.strip() for l in open(p, 'rt').readlines()[3:]]
        self.assertEqual(lines, [
            'DEPTH := .',
            'topsrcdir := %s' % env.topsrcdir,
            'srcdir := %s' % env.topsrcdir,
            'VPATH = %s' % env.topsrcdir,
            '',
            'include $(DEPTH)/config/autoconf.mk',
            '',
            'include $(topsrcdir)/config/rules.mk'
        ])

    def test_backend_mk(self):
        """Ensure backend.mk file is written out properly."""
        env = self._consume('stub0', RecursiveMakeBackend)

        p = os.path.join(env.topobjdir, 'backend.mk')

        lines = [l.strip() for l in open(p, 'rt').readlines()[1:-2]]
        self.assertEqual(lines, [
            'DIRS := dir1',
            'PARALLEL_DIRS := dir2',
            'TEST_DIRS := dir3',
        ])

    def test_no_mtime_bump(self):
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


if __name__ == '__main__':
    main()
