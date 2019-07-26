



from __future__ import unicode_literals

import errno
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

    This is both a wrapper around a file handle as well as a container that
    holds accumulated state.

    It's worth taking a moment to explain the make dependencies. The
    generated backend.mk as well as the Makefile.in (if it exists) are in the
    GLOBAL_DEPS list. This means that if one of them changes, all targets
    in that Makefile are invalidated. backend.mk also depends on all of its
    input files.

    It's worth considering the effect of file mtimes on build behavior.

    Since we perform an "all or none" traversal of moz.build files (the whole
    tree is scanned as opposed to individual files), if we were to blindly
    write backend.mk files, the net effect of updating a single mozbuild file
    in the tree is all backend.mk files have new mtimes. This would in turn
    invalidate all make targets across the whole tree! This would effectively
    undermine incremental builds as any mozbuild change would cause the entire
    tree to rebuild!

    The solution is to not update the mtimes of backend.mk files unless they
    actually change. We use FileAvoidWrite to accomplish this. However, this
    puts us in a somewhat complicated position when it comes to tree recursion.
    As you are recursing the tree, the first time you come across a backend.mk
    that is out of date, a full tree build will be incurred. In typical make
    build systems, we would touch the out-of-date target (backend.mk) to ensure
    its mtime is newer than all its dependencies - even if the contents did
    not change. However, we can't rely on just this approach. During recursion,
    the first trigger of backend generation will cause only that backend.mk to
    update. If there is another backend.mk that is also out of date according
    to mtime but whose contents were not changed, when we recurse to that
    directory, make will trigger another full backend generation! This would
    be completely redundant and would slow down builds! This is not acceptable.

    We work around this problem by having backend generation update the mtime
    of backend.mk if they are older than their inputs - even if the file
    contents did not change. This is essentially a middle ground between
    always updating backend.mk and only updating the backend.mk that was out
    of date during recursion.
    """

    def __init__(self, srcdir, objdir):
        self.srcdir = srcdir
        self.objdir = objdir
        self.path = os.path.join(objdir, 'backend.mk')

        
        self.inputs = set()

        self.fh = FileAvoidWrite(self.path)
        self.fh.write('# THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.\n')
        self.fh.write('\n')
        self.fh.write('MOZBUILD_DERIVED := 1\n')

        
        self.fh.write('NO_MAKEFILE_RULE := 1\n')


    def write(self, buf):
        self.fh.write(buf)

    def close(self):
        if self.inputs:
            l = ' '.join(sorted(self.inputs))
            self.fh.write('BACKEND_INPUT_FILES += %s\n' % l)

        self.fh.close()

        if not self.inputs:
            return

        
        
        existing_mtime = os.path.getmtime(self.path)

        def mtime(path):
            try:
                return os.path.getmtime(path)
            except OSError as e:
                if e.errno == errno.ENOENT:
                    return 0

                raise

        input_mtime = max(mtime(path) for path in self.inputs)

        if input_mtime > existing_mtime:
            os.utime(self.path, None)


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

        
        
        
        autoconf_path = os.path.join(obj.topobjdir, 'config', 'autoconf.mk')
        backend_file.inputs.add(autoconf_path)
        backend_file.inputs |= obj.sandbox_all_paths

        if isinstance(obj, DirectoryTraversal):
            self._process_directory_traversal(obj, backend_file)
        elif isinstance(obj, ConfigFileSubstitution):
            backend_file.write('SUBSTITUTE_FILES += %s\n' % obj.relpath)

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

            bf.write('SUBSTITUTE_FILES += Makefile\n')
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

                static = ' '.join(obj.tier_static_dirs[tier])
                fh.write('EXTERNAL_DIRS += %s\n' % static)

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

