



from __future__ import unicode_literals

import logging
import os

from .base import BuildBackend
from ..frontend.data import (
    ConfigFileSubstitution,
    DirectoryTraversal,
)
from ..util import FileAvoidWrite


class BackendMakeFile(object):
    """Represents a generated backend.mk file.

    This is both a wrapper around FileAvoidWrite as well as a container that
    holds accumulated state.
    """

    def __init__(self, srcdir, objdir):
        self.srcdir = srcdir
        self.objdir = objdir

        
        self.inputs = set()

        
        self.outputs = set()

        self.fh = FileAvoidWrite(os.path.join(objdir, 'backend.mk'))
        self.fh.write('# THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.\n')

    def write(self, buf):
        self.fh.write(buf)

    def close(self):
        if len(self.inputs):
            self.fh.write('BACKEND_INPUT_FILES += %s\n' % ' '.join(self.inputs))

        if len(self.outputs):
            self.fh.write('BACKEND_OUTPUT_FILES += %s\n' % ' '.join(self.outputs))

        self.fh.close()


class RecursiveMakeBackend(BuildBackend):
    """Backend that integrates with the existing recursive make build system.

    This backend facilitates the transition from Makefile.in to moz.build
    files.

    This backend performs Makefile.in -> Makefile conversion. It also writes
    out .mk files containing content derived from moz.build files. Both are
    consumed by the recursive make builder.

    This backend may eventually evolve to write out non-recursive make files.
    However, as long as there are Makefile.in files in the tree, we are tied to
    recursive make and thus will need this backend.
    """

    def _init(self):
        self._backend_files = {}

    def consume_object(self, obj):
        """Write out build files necessary to build with recursive make."""

        backend_file = self._backend_files.get(obj.srcdir,
            BackendMakeFile(obj.srcdir, obj.objdir))

        backend_file.inputs |= obj.sandbox_all_paths

        if isinstance(obj, DirectoryTraversal):
            self._process_directory_traversal(obj, backend_file)
        elif isinstance(obj, ConfigFileSubstitution):
            backend_file.inputs.add(obj.input_path)
            backend_file.outputs.add(obj.output_path)
            self.environment.create_config_file(obj.output_path)

        self._backend_files[obj.srcdir] = backend_file

    def consume_finished(self):
        for srcdir in sorted(self._backend_files.keys()):
            bf = self._backend_files[srcdir]

            if not os.path.exists(bf.objdir):
                os.makedirs(bf.objdir)

            makefile_in = os.path.join(srcdir, 'Makefile.in')

            if not os.path.exists(makefile_in):
                raise Exception('Could not find Makefile.in: %s' % makefile_in)

            out_path = os.path.join(bf.objdir, 'Makefile')
            self.log(logging.DEBUG, 'create_makefile', {'path': out_path},
                'Generating makefile: {path}')
            self.environment.create_config_file(out_path)
            bf.outputs.add(out_path)

            bf.close()

    def _process_directory_traversal(self, obj, backend_file):
        """Process a data.DirectoryTraversal instance."""
        fh = backend_file.fh

        for tier, dirs in obj.tier_dirs.iteritems():
            fh.write('TIERS += %s\n' % tier)

            if dirs:
                fh.write('tier_%s_dirs += %s\n' % (tier, ' '.join(dirs)))

            
            if obj.tier_static_dirs[tier]:
                fh.write('tier_%s_staticdirs += %s\n' % (
                    tier, ' '.join(obj.tier_static_dirs[tier])))

        if obj.dirs:
            fh.write('DIRS := %s\n' % ' '.join(obj.dirs))

        if obj.parallel_dirs:
            fh.write('PARALLEL_DIRS := %s\n' % ' '.join(obj.parallel_dirs))

        if obj.tool_dirs:
            fh.write('TOOL_DIRS := %s\n' % ' '.join(obj.tool_dirs))

        if obj.test_dirs:
            fh.write('TEST_DIRS := %s\n' % ' '.join(obj.test_dirs))

        if obj.test_tool_dirs:
            fh.write('ifdef ENABLE_TESTS\n')
            fh.write('TOOL_DIRS += %s\n' % ' '.join(obj.test_tool_dirs))
            fh.write('endif\n')

        if len(obj.external_make_dirs):
            fh.write('DIRS += %s\n' % ' '.join(obj.external_make_dirs))

        if len(obj.parallel_external_make_dirs):
            fh.write('PARALLEL_DIRS += %s\n' %
                ' '.join(obj.parallel_external_make_dirs))

