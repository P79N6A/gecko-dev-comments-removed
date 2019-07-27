



from __future__ import unicode_literals

import itertools
import json
import logging
import os
import re
import types

from collections import (
    defaultdict,
    namedtuple,
)

import mozwebidlcodegen
from reftest import ReftestManifest

import mozbuild.makeutil as mozmakeutil
from mozpack.copier import FilePurger
from mozpack.manifests import (
    InstallManifest,
)
import mozpack.path as mozpath

from .common import CommonBackend
from ..frontend.data import (
    AndroidEclipseProjectData,
    ConfigFileSubstitution,
    Defines,
    DirectoryTraversal,
    Exports,
    ExternalLibrary,
    GeneratedInclude,
    HostLibrary,
    HostProgram,
    HostSimpleProgram,
    InstallationTarget,
    IPDLFile,
    JARManifest,
    JavaJarData,
    JavaScriptModules,
    Library,
    LocalInclude,
    PerSourceFlag,
    Program,
    Resources,
    SandboxDerived,
    SandboxWrapped,
    SharedLibrary,
    SimpleProgram,
    StaticLibrary,
    TestManifest,
    VariablePassthru,
    XPIDLFile,
)
from ..util import (
    ensureParentDir,
    FileAvoidWrite,
)
from ..makeutil import Makefile

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

    def __init__(self, srcdir, objdir, environment, topobjdir):
        self.srcdir = srcdir
        self.objdir = objdir
        self.relobjdir = mozpath.relpath(objdir, topobjdir)
        self.environment = environment
        self.name = mozpath.join(objdir, 'backend.mk')

        
        self.idls = []
        self.xpt_name = None

        self.fh = FileAvoidWrite(self.name, capture_diff=True)
        self.fh.write('# THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.\n')
        self.fh.write('\n')

    def write(self, buf):
        self.fh.write(buf)

    def write_once(self, buf):
        if '\n' + buf not in self.fh.getvalue():
            self.write(buf)

    
    def add_statement(self, stmt):
        self.write('%s\n' % stmt)

    def close(self):
        if self.xpt_name:
            self.fh.write('XPT_NAME := %s\n' % self.xpt_name)

            
            
            self.fh.write('NONRECURSIVE_TARGETS += export\n')
            self.fh.write('NONRECURSIVE_TARGETS_export += xpidl\n')
            self.fh.write('NONRECURSIVE_TARGETS_export_xpidl_DIRECTORY = '
                '$(DEPTH)/xpcom/xpidl\n')
            self.fh.write('NONRECURSIVE_TARGETS_export_xpidl_TARGETS += '
                'export\n')

        return self.fh.close()

    @property
    def diff(self):
        return self.fh.diff


class RecursiveMakeTraversal(object):
    """
    Helper class to keep track of how the "traditional" recursive make backend
    recurses subdirectories. This is useful until all adhoc rules are removed
    from Makefiles.

    Each directory may have one or more types of subdirectories:
        - (normal) dirs
        - tests
    """
    SubDirectoryCategories = ['dirs', 'tests']
    SubDirectoriesTuple = namedtuple('SubDirectories', SubDirectoryCategories)
    class SubDirectories(SubDirectoriesTuple):
        def __new__(self):
            return RecursiveMakeTraversal.SubDirectoriesTuple.__new__(self, [], [])

    def __init__(self):
        self._traversal = {}

    def add(self, dir, dirs=[], tests=[]):
        """
        Adds a directory to traversal, registering its subdirectories,
        sorted by categories. If the directory was already added to
        traversal, adds the new subdirectories to the already known lists.
        """
        subdirs = self._traversal.setdefault(dir, self.SubDirectories())
        for key, value in (('dirs', dirs), ('tests', tests)):
            assert(key in self.SubDirectoryCategories)
            getattr(subdirs, key).extend(value)

    @staticmethod
    def default_filter(current, subdirs):
        """
        Default filter for use with compute_dependencies and traverse.
        """
        return current, [], subdirs.dirs + subdirs.tests

    def call_filter(self, current, filter):
        """
        Helper function to call a filter from compute_dependencies and
        traverse.
        """
        return filter(current, self._traversal.get(current,
            self.SubDirectories()))

    def compute_dependencies(self, filter=None):
        """
        Compute make dependencies corresponding to the registered directory
        traversal.

        filter is a function with the following signature:
            def filter(current, subdirs)
        where current is the directory being traversed, and subdirs the
        SubDirectories instance corresponding to it.
        The filter function returns a tuple (filtered_current, filtered_parallel,
        filtered_dirs) where filtered_current is either current or None if
        the current directory is to be skipped, and filtered_parallel and
        filtered_dirs are lists of parallel directories and sequential
        directories, which can be rearranged from whatever is given in the
        SubDirectories members.

        The default filter corresponds to a default recursive traversal.
        """
        filter = filter or self.default_filter

        deps = {}

        def recurse(start_node, prev_nodes=None):
            current, parallel, sequential = self.call_filter(start_node, filter)
            if current is not None:
                if start_node != '':
                    deps[start_node] = prev_nodes
                prev_nodes = (start_node,)
            if not start_node in self._traversal:
                return prev_nodes
            parallel_nodes = []
            for node in parallel:
                nodes = recurse(node, prev_nodes)
                if nodes and nodes != ('',):
                    parallel_nodes.extend(nodes)
            if parallel_nodes:
                prev_nodes = tuple(parallel_nodes)
            for dir in sequential:
                prev_nodes = recurse(dir, prev_nodes)
            return prev_nodes

        return recurse(''), deps

    def traverse(self, start, filter=None):
        """
        Iterate over the filtered subdirectories, following the traditional
        make traversal order.
        """
        if filter is None:
            filter = self.default_filter

        current, parallel, sequential = self.call_filter(start, filter)
        if current is not None:
            yield start
        if not start in self._traversal:
            return
        for node in parallel:
            for n in self.traverse(node, filter):
                yield n
        for dir in sequential:
            for d in self.traverse(dir, filter):
                yield d

    def get_subdirs(self, dir):
        """
        Returns all direct subdirectories under the given directory.
        """
        return self._traversal.get(dir, self.SubDirectories())


class RecursiveMakeBackend(CommonBackend):
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
        CommonBackend._init(self)

        self._backend_files = {}
        self._ipdl_sources = set()

        def detailed(summary):
            s = '{:d} total backend files; ' \
                '{:d} created; {:d} updated; {:d} unchanged; ' \
                '{:d} deleted; {:d} -> {:d} Makefile'.format(
                summary.created_count + summary.updated_count +
                summary.unchanged_count,
                summary.created_count,
                summary.updated_count,
                summary.unchanged_count,
                summary.deleted_count,
                summary.makefile_in_count,
                summary.makefile_out_count)

            return s

        
        self.summary.backend_detailed_summary = types.MethodType(detailed,
            self.summary)
        self.summary.makefile_in_count = 0
        self.summary.makefile_out_count = 0

        self._test_manifests = {}

        self.backend_input_files.add(mozpath.join(self.environment.topobjdir,
            'config', 'autoconf.mk'))

        self._install_manifests = {
            k: InstallManifest() for k in [
                'dist_bin',
                'dist_idl',
                'dist_include',
                'dist_public',
                'dist_private',
                'dist_sdk',
                'tests',
                'xpidl',
            ]}

        self._traversal = RecursiveMakeTraversal()
        self._compile_graph = defaultdict(set)
        self._triggers = defaultdict(set)

        self._may_skip = {
            'export': set(),
            'libs': set(),
        }
        self._no_skip = {
            'tools': set(),
        }

    def consume_object(self, obj):
        """Write out build files necessary to build with recursive make."""

        if not isinstance(obj, SandboxDerived):
            return

        if obj.objdir not in self._backend_files:
            self._backend_files[obj.objdir] = \
                BackendMakeFile(obj.srcdir, obj.objdir, obj.config,
                    self.environment.topobjdir)
        backend_file = self._backend_files[obj.objdir]

        CommonBackend.consume_object(self, obj)

        
        
        if isinstance(obj, XPIDLFile):
            backend_file.idls.append(obj)
            backend_file.xpt_name = '%s.xpt' % obj.module

        elif isinstance(obj, TestManifest):
            self._process_test_manifest(obj, backend_file)

        
        if obj._ack:
            return

        if isinstance(obj, DirectoryTraversal):
            self._process_directory_traversal(obj, backend_file)
        elif isinstance(obj, ConfigFileSubstitution):
            
            
            assert os.path.basename(obj.output_path) == 'Makefile'
            self._create_makefile(obj)
        elif isinstance(obj, VariablePassthru):
            unified_suffixes = dict(
                UNIFIED_CSRCS='c',
                UNIFIED_CMMSRCS='mm',
                UNIFIED_CPPSRCS='cpp',
            )

            files_per_unification = 16
            if 'FILES_PER_UNIFIED_FILE' in obj.variables.keys():
                files_per_unification = obj.variables['FILES_PER_UNIFIED_FILE']

            do_unify = not self.environment.substs.get(
                'MOZ_DISABLE_UNIFIED_COMPILATION') and files_per_unification > 1

            
            for k, v in sorted(obj.variables.items()):
                if k in unified_suffixes:
                    if do_unify:
                        
                        
                        unified_prefix = mozpath.relpath(backend_file.objdir,
                            backend_file.environment.topobjdir)
                        if len(unified_prefix) > 20:
                            unified_prefix = unified_prefix[-20:].split('/', 1)[-1]
                        unified_prefix = unified_prefix.replace('/', '_')

                        self._add_unified_build_rules(backend_file, v,
                            backend_file.objdir,
                            unified_prefix='Unified_%s_%s' % (
                                unified_suffixes[k],
                                unified_prefix),
                            unified_suffix=unified_suffixes[k],
                            unified_files_makefile_variable=k,
                            include_curdir_build_rules=False,
                            files_per_unified_file=files_per_unification)
                        backend_file.write('%s += $(%s)\n' % (k[len('UNIFIED_'):], k))
                    else:
                        backend_file.write('%s += %s\n' % (
                            k[len('UNIFIED_'):], ' '.join(sorted(v))))
                elif isinstance(v, list):
                    for item in v:
                        backend_file.write('%s += %s\n' % (k, item))
                elif isinstance(v, bool):
                    if v:
                        backend_file.write('%s := 1\n' % k)
                else:
                    backend_file.write('%s := %s\n' % (k, v))

        elif isinstance(obj, Defines):
            defines = obj.get_defines()
            if defines:
                backend_file.write('DEFINES +=')
                for define in defines:
                    backend_file.write(' %s' % define)
                backend_file.write('\n')

        elif isinstance(obj, Exports):
            self._process_exports(obj, obj.exports, backend_file)

        elif isinstance(obj, Resources):
            self._process_resources(obj, obj.resources, backend_file)

        elif isinstance(obj, JARManifest):
            backend_file.write('JAR_MANIFEST := %s\n' % obj.path)

        elif isinstance(obj, IPDLFile):
            self._ipdl_sources.add(mozpath.join(obj.srcdir, obj.basename))

        elif isinstance(obj, Program):
            self._process_program(obj.program, backend_file)
            self._process_linked_libraries(obj, backend_file)

        elif isinstance(obj, HostProgram):
            self._process_host_program(obj.program, backend_file)
            self._process_linked_libraries(obj, backend_file)

        elif isinstance(obj, SimpleProgram):
            self._process_simple_program(obj, backend_file)
            self._process_linked_libraries(obj, backend_file)

        elif isinstance(obj, HostSimpleProgram):
            self._process_host_simple_program(obj.program, backend_file)
            self._process_linked_libraries(obj, backend_file)

        elif isinstance(obj, LocalInclude):
            self._process_local_include(obj.path, backend_file)

        elif isinstance(obj, GeneratedInclude):
            self._process_generated_include(obj.path, backend_file)

        elif isinstance(obj, PerSourceFlag):
            self._process_per_source_flag(obj, backend_file)

        elif isinstance(obj, InstallationTarget):
            self._process_installation_target(obj, backend_file)

        elif isinstance(obj, JavaScriptModules):
            self._process_javascript_modules(obj, backend_file)

        elif isinstance(obj, SandboxWrapped):
            
            
            
            
            
            if isinstance(obj.wrapped, JavaJarData):
                self._process_java_jar_data(obj.wrapped, backend_file)
            elif isinstance(obj.wrapped, AndroidEclipseProjectData):
                self._process_android_eclipse_project_data(obj.wrapped, backend_file)
            else:
                return

        elif isinstance(obj, SharedLibrary):
            self._process_shared_library(obj, backend_file)
            self._process_linked_libraries(obj, backend_file)

        elif isinstance(obj, StaticLibrary):
            self._process_static_library(obj, backend_file)
            self._process_linked_libraries(obj, backend_file)

        elif isinstance(obj, HostLibrary):
            self._process_host_library(obj, backend_file)
            self._process_linked_libraries(obj, backend_file)

        else:
            return
        obj.ack()

    def _fill_root_mk(self):
        """
        Create two files, root.mk and root-deps.mk, the first containing
        convenience variables, and the other dependency definitions for a
        hopefully proper directory traversal.
        """
        for tier, skip in self._may_skip.items():
            self.log(logging.DEBUG, 'fill_root_mk', {
                'number': len(skip), 'tier': tier
                }, 'Ignoring {number} directories during {tier}')
        for tier, no_skip in self._no_skip.items():
            self.log(logging.DEBUG, 'fill_root_mk', {
                'number': len(no_skip), 'tier': tier
                }, 'Using {number} directories during {tier}')

        
        def parallel_filter(current, subdirs):
            all_subdirs = subdirs.dirs + subdirs.tests
            if current in self._may_skip[tier] \
                    or current.startswith('subtiers/'):
                current = None
            return current, all_subdirs, []

        
        
        
        def libs_filter(current, subdirs):
            if current in self._may_skip['libs'] \
                    or current.startswith('subtiers/'):
                current = None
            return current, [], subdirs.dirs + subdirs.tests

        
        
        
        def tools_filter(current, subdirs):
            if current not in self._no_skip['tools'] \
                    or current.startswith('subtiers/'):
                current = None
            return current, [], subdirs.dirs + subdirs.tests

        filters = [
            ('export', parallel_filter),
            ('libs', libs_filter),
            ('tools', tools_filter),
        ]

        root_deps_mk = Makefile()

        
        for tier, filter in filters:
            main, all_deps = \
                self._traversal.compute_dependencies(filter)
            for dir, deps in all_deps.items():
                if deps is not None:
                    rule = root_deps_mk.create_rule(['%s/%s' % (dir, tier)])
                    rule.add_dependencies('%s/%s' % (d, tier) for d in deps if d)
            rule = root_deps_mk.create_rule(['recurse_%s' % tier])
            if main:
                rule.add_dependencies('%s/%s' % (d, tier) for d in main)

        all_compile_deps = reduce(lambda x,y: x|y,
            self._compile_graph.values()) if self._compile_graph else set()
        compile_roots = set(self._compile_graph.keys()) - all_compile_deps

        rule = root_deps_mk.create_rule(['recurse_compile'])
        rule.add_dependencies(compile_roots)
        for target, deps in sorted(self._compile_graph.items()):
            if deps:
                rule = root_deps_mk.create_rule([target])
                rule.add_dependencies(deps)

        root_mk = Makefile()

        
        for tier, filter in filters:
            all_dirs = self._traversal.traverse('', filter)
            root_mk.add_statement('%s_dirs := %s' % (tier, ' '.join(all_dirs)))

        
        
        root_mk.add_statement('compile_targets := %s' % ' '.join(sorted(
            set(self._compile_graph.keys()) | all_compile_deps)))

        root_mk.add_statement('$(call include_deps,root-deps.mk)')

        with self._write_file(
                mozpath.join(self.environment.topobjdir, 'root.mk')) as root:
            root_mk.dump(root, removal_guard=False)

        with self._write_file(
                mozpath.join(self.environment.topobjdir, 'root-deps.mk')) as root_deps:
            root_deps_mk.dump(root_deps, removal_guard=False)

    def _add_unified_build_rules(self, makefile, files, output_directory,
                                 unified_prefix='Unified',
                                 unified_suffix='cpp',
                                 extra_dependencies=[],
                                 unified_files_makefile_variable='unified_files',
                                 include_curdir_build_rules=True,
                                 poison_windows_h=False,
                                 files_per_unified_file=16):

        
        files = sorted(files)

        explanation = "\n" \
            "# We build files in 'unified' mode by including several files\n" \
            "# together into a single source file.  This cuts down on\n" \
            "# compilation times and debug information size.  %d was chosen as\n" \
            "# a reasonable compromise between clobber rebuild time, incremental\n" \
            "# rebuild time, and compiler memory usage." % files_per_unified_file
        makefile.add_statement(explanation)

        def unified_files():
            "Return an iterator of (unified_filename, source_filenames) tuples."
            
            
            
            dummy_fill_value = ("dummy",)
            def filter_out_dummy(iterable):
                return itertools.ifilter(lambda x: x != dummy_fill_value,
                                         iterable)

            
            def grouper(n, iterable):
                "grouper(3, 'ABCDEFG', 'x') --> ABC DEF Gxx"
                args = [iter(iterable)] * n
                return itertools.izip_longest(fillvalue=dummy_fill_value, *args)

            for i, unified_group in enumerate(grouper(files_per_unified_file,
                                                      files)):
                just_the_filenames = list(filter_out_dummy(unified_group))
                yield '%s%d.%s' % (unified_prefix, i, unified_suffix), just_the_filenames

        all_sources = ' '.join(source for source, _ in unified_files())
        makefile.add_statement('%s := %s' % (unified_files_makefile_variable,
                                               all_sources))

        for unified_file, source_filenames in unified_files():
            if extra_dependencies:
                rule = makefile.create_rule([unified_file])
                rule.add_dependencies(extra_dependencies)

            
            
            
            
            with self._write_file(mozpath.join(output_directory, unified_file)) as f:
                f.write('#define MOZ_UNIFIED_BUILD\n')
                includeTemplate = '#include "%(cppfile)s"'
                if poison_windows_h:
                    includeTemplate += (
                        '\n'
                        '#ifdef _WINDOWS_\n'
                        '#error "%(cppfile)s included windows.h"\n'
                        "#endif")
                includeTemplate += (
                    '\n'
                    '#ifdef PL_ARENA_CONST_ALIGN_MASK\n'
                    '#error "%(cppfile)s uses PL_ARENA_CONST_ALIGN_MASK, '
                    'so it cannot be built in unified mode."\n'
                    '#undef PL_ARENA_CONST_ALIGN_MASK\n'
                    '#endif\n'
                    '#ifdef FORCE_PR_LOG\n'
                    '#error "%(cppfile)s forces NSPR logging, '
                    'so it cannot be built in unified mode."\n'
                    '#undef FORCE_PR_LOG\n'
                    '#endif\n'
                    '#ifdef INITGUID\n'
                    '#error "%(cppfile)s defines INITGUID, '
                    'so it cannot be built in unified mode."\n'
                    '#undef INITGUID\n'
                    '#endif')
                f.write('\n'.join(includeTemplate % { "cppfile": s } for
                                  s in source_filenames))

        if include_curdir_build_rules:
            makefile.add_statement('\n'
                '# Make sometimes gets confused between "foo" and "$(CURDIR)/foo".\n'
                '# Help it out by explicitly specifiying dependencies.')
            makefile.add_statement('all_absolute_unified_files := \\\n'
                                   '  $(addprefix $(CURDIR)/,$(%s))'
                                   % unified_files_makefile_variable)
            rule = makefile.create_rule(['$(all_absolute_unified_files)'])
            rule.add_dependencies(['$(CURDIR)/%: %'])

    def consume_finished(self):
        CommonBackend.consume_finished(self)

        for objdir, backend_file in sorted(self._backend_files.items()):
            srcdir = backend_file.srcdir
            with self._write_file(fh=backend_file) as bf:
                makefile_in = mozpath.join(srcdir, 'Makefile.in')
                makefile = mozpath.join(objdir, 'Makefile')

                
                
                stub = not os.path.exists(makefile_in)
                if not stub:
                    self.log(logging.DEBUG, 'substitute_makefile',
                        {'path': makefile}, 'Substituting makefile: {path}')
                    self.summary.makefile_in_count += 1

                    for tier, skiplist in self._may_skip.items():
                        if bf.relobjdir in skiplist:
                            skiplist.remove(bf.relobjdir)
                else:
                    self.log(logging.DEBUG, 'stub_makefile',
                        {'path': makefile}, 'Creating stub Makefile: {path}')

                obj = self.Substitution()
                obj.output_path = makefile
                obj.input_path = makefile_in
                obj.topsrcdir = bf.environment.topsrcdir
                obj.topobjdir = bf.environment.topobjdir
                obj.config = bf.environment
                self._create_makefile(obj, stub=stub)
                with open(obj.output_path) as fh:
                    content = fh.read()
                    for trigger, targets in self._triggers.items():
                        if trigger.encode('ascii') in content:
                            for target in targets:
                                t = '%s/target' % mozpath.relpath(objdir,
                                    self.environment.topobjdir)
                                self._compile_graph[t].add(target)
                    
                    
                    
                    for t in (b'XPI_PKGNAME', b'INSTALL_EXTENSION_ID',
                            b'tools'):
                        if t not in content:
                            continue
                        if t == b'tools' and not re.search('(?:^|\s)tools.*::', content, re.M):
                            continue
                        if objdir == self.environment.topobjdir:
                            continue
                        self._no_skip['tools'].add(mozpath.relpath(objdir,
                            self.environment.topobjdir))

        
        ipdl_dir = mozpath.join(self.environment.topobjdir, 'ipc', 'ipdl')
        mk = mozmakeutil.Makefile()

        sorted_ipdl_sources = list(sorted(self._ipdl_sources))
        mk.add_statement('ALL_IPDLSRCS := %s' % ' '.join(sorted_ipdl_sources))

        def files_from(ipdl):
            base = mozpath.basename(ipdl)
            root, ext = mozpath.splitext(base)

            
            files = ['%s.cpp' % root]
            if ext == '.ipdl':
                
                files.extend(['%sChild.cpp' % root,
                              '%sParent.cpp' % root])
            return files

        ipdl_cppsrcs = list(itertools.chain(*[files_from(p) for p in sorted_ipdl_sources]))
        self._add_unified_build_rules(mk, ipdl_cppsrcs, ipdl_dir,
                                      unified_prefix='UnifiedProtocols',
                                      unified_files_makefile_variable='CPPSRCS')

        mk.add_statement('IPDLDIRS := %s' % ' '.join(sorted(set(mozpath.dirname(p)
            for p in self._ipdl_sources))))

        with self._write_file(mozpath.join(ipdl_dir, 'ipdlsrcs.mk')) as ipdls:
            mk.dump(ipdls, removal_guard=False)

        self._fill_root_mk()

        
        
        inputs = sorted(p.replace(os.sep, '/') for p in self.backend_input_files)

        
        
        
        with self._write_file('%s.pp' % self._backend_output_list_file) as backend_deps:
            backend_deps.write('$(DEPTH)/backend.%s: %s\n' %
                (self.__class__.__name__, ' '.join(inputs)))
            for path in inputs:
                backend_deps.write('%s:\n' % path)

        with open(self._backend_output_list_file, 'a'):
            pass
        os.utime(self._backend_output_list_file, None)

        
        for flavor, t in self._test_manifests.items():
            install_prefix, manifests = t
            manifest_stem = mozpath.join(install_prefix, '%s.ini' % flavor)
            self._write_master_test_manifest(mozpath.join(
                self.environment.topobjdir, '_tests', manifest_stem),
                manifests)

            
            try:
                self._install_manifests['tests'].add_optional_exists(manifest_stem)
            except ValueError:
                pass

        self._write_manifests('install', self._install_manifests)

        ensureParentDir(mozpath.join(self.environment.topobjdir, 'dist', 'foo'))

    def _process_directory_traversal(self, obj, backend_file):
        """Process a data.DirectoryTraversal instance."""
        fh = backend_file.fh

        def relativize(dirs):
            return [mozpath.normpath(mozpath.join(backend_file.relobjdir, d))
                for d in dirs]

        for tier, dirs in obj.tier_dirs.iteritems():
            fh.write('TIERS += %s\n' % tier)
            
            
            
            
            
            
            
            if dirs:
                fh.write('tier_%s_dirs += %s\n' % (tier, ' '.join(dirs)))
                fh.write('DIRS += $(tier_%s_dirs)\n' % tier)
                self._traversal.add('subtiers/%s' % tier,
                                    dirs=relativize(dirs))

            self._traversal.add('', dirs=['subtiers/%s' % tier])

            for d in dirs:
                if d.trigger:
                    self._triggers[d.trigger].add('%s/target' % d)

        if obj.dirs:
            fh.write('DIRS := %s\n' % ' '.join(obj.dirs))
            self._traversal.add(backend_file.relobjdir, dirs=relativize(obj.dirs))

        if obj.test_dirs:
            fh.write('TEST_DIRS := %s\n' % ' '.join(obj.test_dirs))
            if self.environment.substs.get('ENABLE_TESTS', False):
                self._traversal.add(backend_file.relobjdir,
                                    tests=relativize(obj.test_dirs))

        
        
        self._traversal.add(backend_file.relobjdir)

        affected_tiers = set(obj.affected_tiers)

        for tier in set(self._may_skip.keys()) - affected_tiers:
            self._may_skip[tier].add(backend_file.relobjdir)

    def _process_hierarchy_elements(self, obj, element, namespace, action):
        """Walks the ``HierarchicalStringList`` ``element`` and performs
        ``action`` on each HierarchicalStringList in the hierarchy.

        ``action`` is a callback to be invoked with the following arguments:
        - ``element``     - The HierarchicalStringList along the current namespace
        - ``namespace``   - The namespace of the element
        """
        if namespace:
            namespace += '/'

        action(element, namespace)

        children = element.get_children()
        for subdir in sorted(children):
            self._process_hierarchy_elements(obj, children[subdir],
                                             namespace + subdir,
                                             action)

    def _process_hierarchy(self, obj, element, namespace, action):
        """Walks the ``HierarchicalStringList`` ``element`` and performs
        ``action`` on each string in the hierarchy.

        ``action`` is a callback to be invoked with the following arguments:
        - ``source`` - The path to the source file named by the current string
        - ``dest``   - The relative path, including the namespace, of the
                       destination file.
        """
        def process_element(element, namespace):
            strings = element.get_strings()
            for s in strings:
                source = mozpath.normpath(mozpath.join(obj.srcdir, s))
                dest = '%s%s' % (namespace, mozpath.basename(s))
                flags = None
                if '__getitem__' in dir(element):
                    flags = element[s]
                action(source, dest, flags)

        self._process_hierarchy_elements(obj, element, namespace, process_element)

    def _process_exports(self, obj, exports, backend_file):
        
        
        if not obj.dist_install:
            return

        def handle_export(source, dest, flags):
            self._install_manifests['dist_include'].add_symlink(source, dest)

            if not os.path.exists(source):
                raise Exception('File listed in EXPORTS does not exist: %s' % source)

        self._process_hierarchy(obj, exports,
                                namespace="",
                                action=handle_export)

    def _process_resources(self, obj, resources, backend_file):
        dep_path = mozpath.join(self.environment.topobjdir, '_build_manifests', '.deps', 'install')
        def handle_resource(source, dest, flags):
            if flags.preprocess:
                if dest.endswith('.in'):
                    dest = dest[:-3]
                dep_file = mozpath.join(dep_path, mozpath.basename(source) + '.pp')
                self._install_manifests['dist_bin'].add_preprocess(source, dest, dep_file, marker='%', defines=obj.defines)
            else:
                self._install_manifests['dist_bin'].add_symlink(source, dest)

            if not os.path.exists(source):
                raise Exception('File listed in RESOURCE_FILES does not exist: %s' % source)

        
        
        self._process_hierarchy(obj, resources,
                                namespace='res',
                                action=handle_resource)

    def _process_installation_target(self, obj, backend_file):
        
        
        
        if obj.xpiname:
            backend_file.write('XPI_NAME = %s\n' % (obj.xpiname))
        if obj.subdir:
            backend_file.write('DIST_SUBDIR = %s\n' % (obj.subdir))
        if obj.target and not obj.is_custom():
            backend_file.write('FINAL_TARGET = $(DEPTH)/%s\n' % (obj.target))
        else:
            backend_file.write('FINAL_TARGET = $(if $(XPI_NAME),$(DIST)/xpi-stage/$(XPI_NAME),$(DIST)/bin)$(DIST_SUBDIR:%=/%)\n')

        if not obj.enabled:
            backend_file.write('NO_DIST_INSTALL := 1\n')

    def _process_javascript_modules(self, obj, backend_file):
        if obj.flavor not in ('extra', 'extra_pp', 'testing'):
            raise Exception('Unsupported JavaScriptModules instance: %s' % obj.flavor)

        if obj.flavor == 'extra':
            def onelement(element, namespace):
                if not element.get_strings():
                    return

                prefix = 'extra_js_%s' % namespace.replace('/', '_')
                backend_file.write('%s_FILES := %s\n'
                                   % (prefix, ' '.join(element.get_strings())))
                backend_file.write('%s_DEST = $(FINAL_TARGET)/%s\n' % (prefix, namespace))
                backend_file.write('INSTALL_TARGETS += %s\n\n' % prefix)
            self._process_hierarchy_elements(obj, obj.modules, 'modules', onelement)
            return

        if obj.flavor == 'extra_pp':
            def onelement(element, namespace):
                if not element.get_strings():
                    return

                prefix = 'extra_pp_js_%s' % namespace.replace('/', '_')
                backend_file.write('%s := %s\n'
                                   % (prefix, ' '.join(element.get_strings())))
                backend_file.write('%s_PATH = $(FINAL_TARGET)/%s\n' % (prefix, namespace))
                backend_file.write('PP_TARGETS += %s\n\n' % prefix)
            self._process_hierarchy_elements(obj, obj.modules, 'modules', onelement)
            return

        if not self.environment.substs.get('ENABLE_TESTS', False):
            return

        manifest = self._install_manifests['tests']

        def onmodule(source, dest, flags):
            manifest.add_symlink(source, mozpath.join('modules', dest))

        self._process_hierarchy(obj, obj.modules, '', onmodule)

    def _handle_idl_manager(self, manager):
        build_files = self._install_manifests['xpidl']

        for p in ('Makefile', 'backend.mk', '.deps/.mkdir.done',
            'xpt/.mkdir.done'):
            build_files.add_optional_exists(p)

        for idl in manager.idls.values():
            self._install_manifests['dist_idl'].add_symlink(idl['source'],
                idl['basename'])
            self._install_manifests['dist_include'].add_optional_exists('%s.h'
                % idl['root'])

        for module in manager.modules:
            build_files.add_optional_exists(mozpath.join('xpt',
                '%s.xpt' % module))
            build_files.add_optional_exists(mozpath.join('.deps',
                '%s.pp' % module))

        modules = manager.modules
        xpt_modules = sorted(modules.keys())
        rules = []

        for module in xpt_modules:
            deps = sorted(modules[module])
            idl_deps = ['$(dist_idl_dir)/%s.idl' % dep for dep in deps]
            rules.extend([
                
                
                
                
                
                
                
                
                
                
                '$(idl_xpt_dir)/%s.xpt: %s' % (module, ' '.join(idl_deps)),
                '\t@echo "$(notdir $@)"',
                '\t$(idlprocess) $(basename $(notdir $@)) %s' % ' '.join(deps),
                '',
            ])

        
        
        
        

        obj = self.Substitution()
        obj.output_path = mozpath.join(self.environment.topobjdir, 'config',
            'makefiles', 'xpidl', 'Makefile')
        obj.input_path = mozpath.join(self.environment.topsrcdir, 'config',
            'makefiles', 'xpidl', 'Makefile.in')
        obj.topsrcdir = self.environment.topsrcdir
        obj.topobjdir = self.environment.topobjdir
        obj.config = self.environment
        self._create_makefile(obj, extra=dict(
            xpidl_rules='\n'.join(rules),
            xpidl_modules=' '.join(xpt_modules),
        ))

    def _process_program(self, program, backend_file):
        backend_file.write('PROGRAM = %s\n' % program)

    def _process_host_program(self, program, backend_file):
        backend_file.write('HOST_PROGRAM = %s\n' % program)

    def _process_simple_program(self, obj, backend_file):
        if obj.is_unit_test:
            backend_file.write('CPP_UNIT_TESTS += %s\n' % obj.program)
            
            
            if 'NSPR_LIBS' in self._triggers:
                self._compile_graph[self._build_target_for_obj(obj)] |= \
                    self._triggers['NSPR_LIBS']
        else:
            backend_file.write('SIMPLE_PROGRAMS += %s\n' % obj.program)

    def _process_host_simple_program(self, program, backend_file):
        backend_file.write('HOST_SIMPLE_PROGRAMS += %s\n' % program)

    def _process_test_manifest(self, obj, backend_file):
        
        self.backend_input_files.add(mozpath.join(obj.topsrcdir,
            obj.manifest_relpath))

        
        
        
        for source, (dest, is_test) in obj.installs.items():
            try:
                self._install_manifests['tests'].add_symlink(source, dest)
            except ValueError:
                if not obj.dupe_manifest and is_test:
                    raise

        for base, pattern, dest in obj.pattern_installs:
            try:
                self._install_manifests['tests'].add_pattern_symlink(base,
                    pattern, dest)
            except ValueError:
                if not obj.dupe_manifest:
                    raise

        for dest in obj.external_installs:
            try:
                self._install_manifests['tests'].add_optional_exists(dest)
            except ValueError:
                if not obj.dupe_manifest:
                    raise

        m = self._test_manifests.setdefault(obj.flavor,
            (obj.install_prefix, set()))
        m[1].add(obj.manifest_obj_relpath)

        if isinstance(obj.manifest, ReftestManifest):
            
            
            self.backend_input_files |= obj.manifest.manifests

    def _process_local_include(self, local_include, backend_file):
        if local_include.startswith('/'):
            path = '$(topsrcdir)'
        else:
            path = '$(srcdir)/'
        backend_file.write('LOCAL_INCLUDES += -I%s%s\n' % (path, local_include))

    def _process_generated_include(self, generated_include, backend_file):
        if generated_include.startswith('/'):
            path = self.environment.topobjdir.replace('\\', '/')
        else:
            path = ''
        backend_file.write('LOCAL_INCLUDES += -I%s%s\n' % (path, generated_include))

    def _process_per_source_flag(self, per_source_flag, backend_file):
        for flag in per_source_flag.flags:
            backend_file.write('%s_FLAGS += %s\n' % (mozpath.basename(per_source_flag.file_name), flag))

    def _process_java_jar_data(self, jar, backend_file):
        target = jar.name
        backend_file.write('JAVA_JAR_TARGETS += %s\n' % target)
        backend_file.write('%s_DEST := %s.jar\n' % (target, jar.name))
        if jar.sources:
            backend_file.write('%s_JAVAFILES := %s\n' %
                (target, ' '.join(jar.sources)))
        if jar.generated_sources:
            backend_file.write('%s_PP_JAVAFILES := %s\n' %
                (target, ' '.join(mozpath.join('generated', f) for f in jar.generated_sources)))
        if jar.extra_jars:
            backend_file.write('%s_EXTRA_JARS := %s\n' %
                (target, ' '.join(jar.extra_jars)))
        if jar.javac_flags:
            backend_file.write('%s_JAVAC_FLAGS := %s\n' %
                (target, ' '.join(jar.javac_flags)))

    def _process_android_eclipse_project_data(self, project, backend_file):
        
        
        
        
        
        

        project_directory = mozpath.join(self.environment.topobjdir, 'android_eclipse', project.name)
        manifest_path = mozpath.join(self.environment.topobjdir, 'android_eclipse', '%s.manifest' % project.name)

        fragment = Makefile()
        rule = fragment.create_rule(targets=['ANDROID_ECLIPSE_PROJECT_%s' % project.name])
        rule.add_dependencies(project.recursive_make_targets)
        args = ['--no-remove',
            '--no-remove-all-directory-symlinks',
            '--no-remove-empty-directories',
            project_directory,
            manifest_path]
        rule.add_commands(['$(call py_action,process_install_manifest,%s)' % ' '.join(args)])
        fragment.dump(backend_file.fh, removal_guard=False)

    def _process_shared_library(self, libdef, backend_file):
        backend_file.write_once('LIBRARY_NAME := %s\n' % libdef.basename)
        backend_file.write('FORCE_SHARED_LIB := 1\n')
        backend_file.write('IMPORT_LIBRARY := %s\n' % libdef.import_name)
        backend_file.write('SHARED_LIBRARY := %s\n' % libdef.lib_name)
        if libdef.variant == libdef.COMPONENT:
            backend_file.write('IS_COMPONENT := 1\n')
        if libdef.soname:
            backend_file.write('DSO_SONAME := %s\n' % libdef.soname)
        if libdef.is_sdk:
            backend_file.write('SDK_LIBRARY := %s\n' % libdef.import_name)

    def _process_static_library(self, libdef, backend_file):
        backend_file.write_once('LIBRARY_NAME := %s\n' % libdef.basename)
        backend_file.write('FORCE_STATIC_LIB := 1\n')
        backend_file.write('REAL_LIBRARY := %s\n' % libdef.lib_name)
        if libdef.is_sdk:
            backend_file.write('SDK_LIBRARY := %s\n' % libdef.import_name)

    def _process_host_library(self, libdef, backend_file):
        backend_file.write('HOST_LIBRARY_NAME = %s\n' % libdef.basename)

    def _build_target_for_obj(self, obj):
        return '%s/%s' % (mozpath.relpath(obj.objdir,
            self.environment.topobjdir), obj.KIND)

    def _process_linked_libraries(self, obj, backend_file):
        def write_shared_and_system_libs(lib):
            for l in lib.linked_libraries:
                if isinstance(l, StaticLibrary):
                    write_shared_and_system_libs(l)
                else:
                    backend_file.write_once('SHARED_LIBS += %s/%s\n'
                        % (pretty_relpath(l), l.import_name))
            for l in lib.linked_system_libs:
                backend_file.write_once('OS_LIBS += %s\n' % l)

        def pretty_relpath(lib):
            return '$(DEPTH)/%s' % mozpath.relpath(lib.objdir, topobjdir)

        topobjdir = mozpath.normsep(obj.topobjdir)
        
        build_target = self._build_target_for_obj(obj)
        self._compile_graph[build_target]

        
        
        
        if obj.KIND == 'target' and not isinstance(obj, StaticLibrary) and \
                build_target != 'mozglue/build/target' and \
                not obj.config.substs.get('JS_STANDALONE'):
            self._compile_graph[build_target].add('mozglue/build/target')
            if obj.config.substs.get('MOZ_MEMORY'):
                self._compile_graph[build_target].add('memory/build/target')

        
        
        if obj.KIND == 'target' and not isinstance(obj, StaticLibrary) and \
                build_target != 'build/stlport/target' and \
                'stlport' in obj.config.substs.get('STLPORT_LIBS'):
            self._compile_graph[build_target].add('build/stlport/target')

        for lib in obj.linked_libraries:
            if not isinstance(lib, ExternalLibrary):
                self._compile_graph[build_target].add(
                    self._build_target_for_obj(lib))
            relpath = pretty_relpath(lib)
            if isinstance(obj, Library):
                if isinstance(lib, StaticLibrary):
                    backend_file.write_once('STATIC_LIBS += %s/%s\n'
                                        % (relpath, lib.import_name))
                    if isinstance(obj, SharedLibrary):
                        write_shared_and_system_libs(lib)
                elif isinstance(obj, SharedLibrary):
                    assert lib.variant != lib.COMPONENT
                    backend_file.write_once('SHARED_LIBS += %s/%s\n'
                                        % (relpath, lib.import_name))
            elif isinstance(obj, (Program, SimpleProgram)):
                if isinstance(lib, StaticLibrary):
                    backend_file.write_once('STATIC_LIBS += %s/%s\n'
                                        % (relpath, lib.import_name))
                    write_shared_and_system_libs(lib)
                else:
                    assert lib.variant != lib.COMPONENT
                    backend_file.write_once('SHARED_LIBS += %s/%s\n'
                                        % (relpath, lib.import_name))
            elif isinstance(obj, (HostLibrary, HostProgram, HostSimpleProgram)):
                assert isinstance(lib, HostLibrary)
                backend_file.write_once('HOST_LIBS += %s/%s\n'
                                   % (relpath, lib.import_name))

        for lib in obj.linked_system_libs:
            if obj.KIND == 'target':
                backend_file.write_once('OS_LIBS += %s\n' % lib)
            else:
                backend_file.write_once('HOST_EXTRA_LIBS += %s\n' % lib)

    def _write_manifests(self, dest, manifests):
        man_dir = mozpath.join(self.environment.topobjdir, '_build_manifests',
            dest)

        
        
        purger = FilePurger()

        for k, manifest in manifests.items():
            purger.add(k)

            with self._write_file(mozpath.join(man_dir, k)) as fh:
                manifest.write(fileobj=fh)

        purger.purge(man_dir)

    def _write_master_test_manifest(self, path, manifests):
        with self._write_file(path) as master:
            master.write(
                '; THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT MODIFY BY HAND.\n\n')

            for manifest in sorted(manifests):
                master.write('[include:%s]\n' % manifest)

    class Substitution(object):
        """BaseConfigSubstitution-like class for use with _create_makefile."""
        __slots__ = (
            'input_path',
            'output_path',
            'topsrcdir',
            'topobjdir',
            'config',
        )

    def _create_makefile(self, obj, stub=False, extra=None):
        '''Creates the given makefile. Makefiles are treated the same as
        config files, but some additional header and footer is added to the
        output.

        When the stub argument is True, no source file is used, and a stub
        makefile with the default header and footer only is created.
        '''
        with self._get_preprocessor(obj) as pp:
            if extra:
                pp.context.update(extra)
            if not pp.context.get('autoconfmk', ''):
                pp.context['autoconfmk'] = 'autoconf.mk'
            pp.handleLine(b'# THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT MODIFY BY HAND.\n');
            pp.handleLine(b'DEPTH := @DEPTH@\n')
            pp.handleLine(b'topsrcdir := @top_srcdir@\n')
            pp.handleLine(b'srcdir := @srcdir@\n')
            pp.handleLine(b'VPATH := @srcdir@\n')
            pp.handleLine(b'relativesrcdir := @relativesrcdir@\n')
            pp.handleLine(b'include $(DEPTH)/config/@autoconfmk@\n')
            if not stub:
                pp.do_include(obj.input_path)
            
            
            pp.handleLine(b'\n')
            pp.handleLine(b'include $(topsrcdir)/config/recurse.mk\n')
        if not stub:
            
            
            
            
            
            self.backend_input_files.add(obj.input_path)

        self.summary.makefile_out_count += 1

    def _handle_webidl_collection(self, webidls):
        if not webidls.all_stems():
            return

        bindings_dir = mozpath.join(self.environment.topobjdir, 'dom',
            'bindings')

        all_inputs = set(webidls.all_static_sources())
        for s in webidls.all_non_static_basenames():
            all_inputs.add(mozpath.join(bindings_dir, s))

        generated_events_stems = webidls.generated_events_stems()
        exported_stems = webidls.all_regular_stems()

        
        
        o = dict(
            webidls=sorted(all_inputs),
            generated_events_stems=sorted(generated_events_stems),
            exported_stems=sorted(exported_stems),
            example_interfaces=sorted(webidls.example_interfaces),
        )

        file_lists = mozpath.join(bindings_dir, 'file-lists.json')
        with self._write_file(file_lists) as fh:
            json.dump(o, fh, sort_keys=True, indent=2)

        manager = mozwebidlcodegen.create_build_system_manager(
            self.environment.topsrcdir,
            self.environment.topobjdir,
            mozpath.join(self.environment.topobjdir, 'dist')
        )

        
        
        include_dir = mozpath.join(self.environment.topobjdir, 'dist',
            'include')
        for f in manager.expected_build_output_files():
            if f.startswith(include_dir):
                self._install_manifests['dist_include'].add_optional_exists(
                    mozpath.relpath(f, include_dir))

        
        mk = Makefile()
        mk.add_statement('nonstatic_webidl_files := %s' % ' '.join(
            sorted(webidls.all_non_static_basenames())))
        mk.add_statement('globalgen_sources := %s' % ' '.join(
            sorted(manager.GLOBAL_DEFINE_FILES)))
        mk.add_statement('test_sources := %s' % ' '.join(
            sorted('%sBinding.cpp' % s for s in webidls.all_test_stems())))

        
        
        
        
        
        
        for source in sorted(webidls.all_preprocessed_sources()):
            basename = os.path.basename(source)
            rule = mk.create_rule([basename])
            rule.add_dependencies([source, '$(GLOBAL_DEPS)'])
            rule.add_commands([
                
                
                
                '$(RM) $@',
                '$(call py_action,preprocessor,$(DEFINES) $(ACDEFINES) '
                    '$(XULPPFLAGS) $< -o $@)'
            ])

        
        
        
        self._add_unified_build_rules(mk,
            webidls.all_regular_cpp_basenames(),
            bindings_dir,
            unified_prefix='UnifiedBindings',
            unified_files_makefile_variable='unified_binding_cpp_files',
            poison_windows_h=True,
            files_per_unified_file=32)

        webidls_mk = mozpath.join(bindings_dir, 'webidlsrcs.mk')
        with self._write_file(webidls_mk) as fh:
            mk.dump(fh, removal_guard=False)
