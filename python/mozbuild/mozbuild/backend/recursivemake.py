



from __future__ import unicode_literals

import errno
import logging
import os
import types

import mozpack.path
from mozpack.copier import FilePurger
from mozpack.manifests import (
    InstallManifest,
    PurgeManifest,
)

from .base import BuildBackend
from ..frontend.data import (
    ConfigFileSubstitution,
    DirectoryTraversal,
    IPDLFile,
    SandboxDerived,
    VariablePassthru,
    Exports,
    Program,
    XpcshellManifests,
)
from ..util import FileAvoidWrite


STUB_MAKEFILE = '''
# THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT MODIFY BY HAND.

DEPTH          := {depth}
topsrcdir      := {topsrc}
srcdir         := {src}
VPATH          := {src}
relativesrcdir := {relsrc}

include {topsrc}/config/rules.mk
'''.lstrip()


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
    actually change. We use FileAvoidWrite to accomplish this.
    """

    def __init__(self, srcdir, objdir, environment):
        self.srcdir = srcdir
        self.objdir = objdir
        self.environment = environment
        self.path = os.path.join(objdir, 'backend.mk')

        self.fh = FileAvoidWrite(self.path)
        self.fh.write('# THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.\n')
        self.fh.write('\n')
        self.fh.write('MOZBUILD_DERIVED := 1\n')

        
        self.fh.write('NO_MAKEFILE_RULE := 1\n')

        
        
        
        
        
        self.fh.write('NO_SUBMAKEFILES_RULE := 1\n')


    def write(self, buf):
        self.fh.write(buf)

    def close(self):
        return self.fh.close()


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
        self._ipdl_sources = set()

        def detailed(summary):
            return '{:d} total backend files. {:d} created; {:d} updated; {:d} unchanged'.format(
                summary.managed_count, summary.created_count,
                summary.updated_count, summary.unchanged_count)

        
        self.summary.backend_detailed_summary = types.MethodType(detailed,
            self.summary)

        self.xpcshell_manifests = []

        self.backend_input_files.add(os.path.join(self.environment.topobjdir,
            'config', 'autoconf.mk'))

        self._install_manifests = dict()

        self._purge_manifests = dict(
            dist_bin=PurgeManifest(relpath='dist/bin'),
            dist_include=PurgeManifest(relpath='dist/include'),
            dist_private=PurgeManifest(relpath='dist/private'),
            dist_public=PurgeManifest(relpath='dist/public'),
            dist_sdk=PurgeManifest(relpath='dist/sdk'),
            tests=PurgeManifest(relpath='_tests'),
        )

    def _update_from_avoid_write(self, result):
        existed, updated = result

        if not existed:
            self.summary.created_count += 1
        elif updated:
            self.summary.updated_count += 1
        else:
            self.summary.unchanged_count += 1

    def consume_object(self, obj):
        """Write out build files necessary to build with recursive make."""

        if not isinstance(obj, SandboxDerived):
            return

        backend_file = self._backend_files.get(obj.srcdir,
            BackendMakeFile(obj.srcdir, obj.objdir, self.get_environment(obj)))

        if isinstance(obj, DirectoryTraversal):
            self._process_directory_traversal(obj, backend_file)
        elif isinstance(obj, ConfigFileSubstitution):
            self._update_from_avoid_write(
                backend_file.environment.create_config_file(obj.output_path))
            self.backend_input_files.add(obj.input_path)
            self.summary.managed_count += 1
        elif isinstance(obj, VariablePassthru):
            
            for k, v in sorted(obj.variables.items()):
                if isinstance(v, list):
                    for item in v:
                        backend_file.write('%s += %s\n' % (k, item))

                else:
                    backend_file.write('%s := %s\n' % (k, v))
        elif isinstance(obj, Exports):
            self._process_exports(obj.exports, backend_file)

        elif isinstance(obj, IPDLFile):
            self._ipdl_sources.add(mozpack.path.join(obj.srcdir, obj.basename))

        elif isinstance(obj, Program):
            self._process_program(obj.program, backend_file)

        elif isinstance(obj, XpcshellManifests):
            self._process_xpcshell_manifests(obj, backend_file)

        self._backend_files[obj.srcdir] = backend_file

    def consume_finished(self):
        for srcdir in sorted(self._backend_files.keys()):
            bf = self._backend_files[srcdir]

            if not os.path.exists(bf.objdir):
                try:
                    os.makedirs(bf.objdir)
                except OSError as error:
                    if error.errno != errno.EEXIST:
                        raise

            makefile_in = os.path.join(srcdir, 'Makefile.in')
            makefile = os.path.join(bf.objdir, 'Makefile')

            
            
            if os.path.exists(makefile_in):
                self.log(logging.DEBUG, 'substitute_makefile',
                    {'path': makefile}, 'Substituting makefile: {path}')

                self._update_from_avoid_write(
                    bf.environment.create_config_file(makefile))
                self.summary.managed_count += 1

                
                
                
                
                
                self.backend_input_files.add(makefile_in)
            else:
                self.log(logging.DEBUG, 'stub_makefile',
                    {'path': makefile}, 'Creating stub Makefile: {path}')

                params = {
                    'topsrc': bf.environment.get_top_srcdir(makefile),
                    'src': bf.environment.get_file_srcdir(makefile),
                    'depth': bf.environment.get_depth(makefile),
                    'relsrc': bf.environment.get_relative_srcdir(makefile),
                }

                aw = FileAvoidWrite(makefile)
                aw.write(STUB_MAKEFILE.format(**params))
                self._update_from_avoid_write(aw.close())
                self.summary.managed_count += 1

            self._update_from_avoid_write(bf.close())
            self.summary.managed_count += 1


        
        ipdls = FileAvoidWrite(os.path.join(self.environment.topobjdir,
            'ipc', 'ipdl', 'ipdlsrcs.mk'))
        for p in sorted(self._ipdl_sources):
            ipdls.write('ALL_IPDLSRCS += %s\n' % p)
            base = os.path.basename(p)
            root, ext = os.path.splitext(base)

            
            ipdls.write('CPPSRCS += %s.cpp\n' % root)
            if ext == '.ipdl':
                
                ipdls.write('CPPSRCS += %sChild.cpp\n' % root)
                ipdls.write('CPPSRCS += %sParent.cpp\n' % root)

        ipdls.write('IPDLDIRS := %s\n' % ' '.join(sorted(set(os.path.dirname(p)
            for p in self._ipdl_sources))))

        self._update_from_avoid_write(ipdls.close())
        self.summary.managed_count += 1

        
        
        backend_built_path = os.path.join(self.environment.topobjdir,
            'backend.%s.built' % self.__class__.__name__).replace(os.sep, '/')
        backend_deps = FileAvoidWrite('%s.pp' % backend_built_path)
        inputs = sorted(p.replace(os.sep, '/') for p in self.backend_input_files)

        
        
        
        backend_deps.write('$(DEPTH)/backend.RecursiveMakeBackend.built: %s\n' %
            ' '.join(inputs))
        for path in inputs:
            backend_deps.write('%s:\n' % path)

        self._update_from_avoid_write(backend_deps.close())
        self.summary.managed_count += 1

        
        self.xpcshell_manifests.sort()
        if len(self.xpcshell_manifests) > 0:
            mastermanifest = FileAvoidWrite(os.path.join(
                self.environment.topobjdir, 'testing', 'xpcshell', 'xpcshell.ini'))
            mastermanifest.write(
                '; THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT MODIFY BY HAND.\n\n')
            for manifest in self.xpcshell_manifests:
                mastermanifest.write("[include:%s]\n" % manifest)
            self._update_from_avoid_write(mastermanifest.close())
            self.summary.managed_count += 1

        self._write_manifests('install', self._install_manifests)
        self._write_manifests('purge', self._purge_manifests)

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

        if obj.test_tool_dirs and \
            self.environment.substs.get('ENABLE_TESTS', False):

            fh.write('TOOL_DIRS += %s\n' % ' '.join(obj.test_tool_dirs))

        if len(obj.external_make_dirs):
            fh.write('DIRS += %s\n' % ' '.join(obj.external_make_dirs))

        if len(obj.parallel_external_make_dirs):
            fh.write('PARALLEL_DIRS += %s\n' %
                ' '.join(obj.parallel_external_make_dirs))

    def _process_exports(self, exports, backend_file, namespace=""):
        strings = exports.get_strings()
        if namespace:
            if strings:
                backend_file.write('EXPORTS_NAMESPACES += %s\n' % namespace)
            export_name = 'EXPORTS_%s' % namespace
            namespace += '/'
        else:
            export_name = 'EXPORTS'

        
        
        if strings:
            backend_file.write('%s += %s\n' % (export_name, ' '.join(strings)))

            for s in strings:
                p = '%s%s' % (namespace, s)
                self._purge_manifests['dist_include'].add(p)

        children = exports.get_children()
        for subdir in sorted(children):
            self._process_exports(children[subdir], backend_file,
                                  namespace=namespace + subdir)

    def _process_program(self, program, backend_file):
        backend_file.write('PROGRAM = %s\n' % program)

    def _process_xpcshell_manifests(self, obj, backend_file, namespace=""):
        manifest = obj.xpcshell_manifests
        backend_file.write('XPCSHELL_TESTS += %s\n' % os.path.dirname(manifest))
        if obj.relativedir != '':
            manifest = '%s/%s' % (obj.relativedir, manifest)
        self.xpcshell_manifests.append(manifest)

    def _write_manifests(self, dest, manifests):
        man_dir = os.path.join(self.environment.topobjdir, '_build_manifests',
            dest)

        
        
        purger = FilePurger()

        for k, manifest in manifests.items():
            purger.add(k)

            fh = FileAvoidWrite(os.path.join(man_dir, k))
            manifest.write(fileobj=fh)
            self._update_from_avoid_write(fh.close())

        purger.purge(man_dir)
