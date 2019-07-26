



from __future__ import unicode_literals

import logging
import os

from mach.mixin.logging import LoggingMixin

import mozpack.path as mozpath

from .data import (
    ConfigFileSubstitution,
    Defines,
    DirectoryTraversal,
    Exports,
    GeneratedEventWebIDLFile,
    GeneratedWebIDLFile,
    IPDLFile,
    LocalInclude,
    PreprocessedWebIDLFile,
    Program,
    ReaderSummary,
    TestWebIDLFile,
    VariablePassthru,
    XPIDLFile,
    XpcshellManifests,
    WebIDLFile,
)

from .reader import (
    MozbuildSandbox,
    SandboxValidationError,
)


class TreeMetadataEmitter(LoggingMixin):
    """Converts the executed mozbuild files into data structures.

    This is a bridge between reader.py and data.py. It takes what was read by
    reader.BuildReader and converts it into the classes defined in the data
    module.
    """

    def __init__(self, config):
        self.populate_logger()

        self.config = config

    def emit(self, output):
        """Convert the BuildReader output into data structures.

        The return value from BuildReader.read_topsrcdir() (a generator) is
        typically fed into this function.
        """
        file_count = 0
        execution_time = 0.0

        for out in output:
            if isinstance(out, MozbuildSandbox):
                for o in self.emit_from_sandbox(out):
                    yield o

                
                file_count += len(out.all_paths)
                execution_time += out.execution_time

            else:
                raise Exception('Unhandled output type: %s' % out)

        yield ReaderSummary(file_count, execution_time)

    def emit_from_sandbox(self, sandbox):
        """Convert a MozbuildSandbox to tree metadata objects.

        This is a generator of mozbuild.frontend.data.SandboxDerived instances.
        """
        
        
        for o in self._emit_directory_traversal_from_sandbox(sandbox): yield o

        for path in sandbox['CONFIGURE_SUBST_FILES']:
            if os.path.isabs(path):
                path = path[1:]

            sub = ConfigFileSubstitution(sandbox)
            sub.input_path = os.path.join(sandbox['SRCDIR'], '%s.in' % path)
            sub.output_path = os.path.join(sandbox['OBJDIR'], path)
            sub.relpath = path
            yield sub

        
        
        
        
        xpidl_module = sandbox['MODULE']
        if sandbox['XPIDL_MODULE']:
            xpidl_module = sandbox['XPIDL_MODULE']

        if sandbox['XPIDL_SOURCES'] and not xpidl_module:
            raise SandboxValidationError('MODULE or XPIDL_MODULE must be '
                'defined if XPIDL_SOURCES is defined.')

        if sandbox['XPIDL_SOURCES'] and sandbox['NO_DIST_INSTALL']:
            self.log(logging.WARN, 'mozbuild_warning', dict(
                path=sandbox.main_path),
                '{path}: NO_DIST_INSTALL has no effect on XPIDL_SOURCES.')

        for idl in sandbox['XPIDL_SOURCES']:
            yield XPIDLFile(sandbox, mozpath.join(sandbox['SRCDIR'], idl),
                xpidl_module)

        
        
        
        passthru = VariablePassthru(sandbox)
        varmap = dict(
            
            ASFILES='ASFILES',
            CMMSRCS='CMMSRCS',
            CPPSRCS='CPP_SOURCES',
            CPP_UNIT_TESTS='CPP_UNIT_TESTS',
            CSRCS='CSRCS',
            EXPORT_LIBRARY='EXPORT_LIBRARY',
            EXTRA_COMPONENTS='EXTRA_COMPONENTS',
            EXTRA_JS_MODULES='EXTRA_JS_MODULES',
            EXTRA_PP_COMPONENTS='EXTRA_PP_COMPONENTS',
            EXTRA_PP_JS_MODULES='EXTRA_PP_JS_MODULES',
            FAIL_ON_WARNINGS='FAIL_ON_WARNINGS',
            FORCE_SHARED_LIB='FORCE_SHARED_LIB',
            FORCE_STATIC_LIB='FORCE_STATIC_LIB',
            GTEST_CMMSRCS='GTEST_CMM_SOURCES',
            GTEST_CPPSRCS='GTEST_CPP_SOURCES',
            GTEST_CSRCS='GTEST_C_SOURCES',
            HOST_CPPSRCS='HOST_CPPSRCS',
            HOST_CSRCS='HOST_CSRCS',
            HOST_LIBRARY_NAME='HOST_LIBRARY_NAME',
            IS_COMPONENT='IS_COMPONENT',
            JS_MODULES_PATH='JS_MODULES_PATH',
            LIBRARY_NAME='LIBRARY_NAME',
            LIBS='LIBS',
            LIBXUL_LIBRARY='LIBXUL_LIBRARY',
            MODULE='MODULE',
            MSVC_ENABLE_PGO='MSVC_ENABLE_PGO',
            NO_DIST_INSTALL='NO_DIST_INSTALL',
            OS_LIBS='OS_LIBS',
            SDK_LIBRARY='SDK_LIBRARY',
            SHARED_LIBRARY_LIBS='SHARED_LIBRARY_LIBS',
            SIMPLE_PROGRAMS='SIMPLE_PROGRAMS',
            SSRCS='SSRCS',
        )
        for mak, moz in varmap.items():
            if sandbox[moz]:
                passthru.variables[mak] = sandbox[moz]

        if passthru.variables:
            yield passthru

        exports = sandbox.get('EXPORTS')
        if exports:
            yield Exports(sandbox, exports,
                dist_install=not sandbox.get('NO_DIST_INSTALL', False))

        defines = sandbox.get('DEFINES')
        if defines:
            yield Defines(sandbox, defines)

        program = sandbox.get('PROGRAM')
        if program:
            yield Program(sandbox, program, sandbox['CONFIG']['BIN_SUFFIX'])

        simple_lists = [
            ('GENERATED_EVENTS_WEBIDL_FILES', GeneratedEventWebIDLFile),
            ('GENERATED_WEBIDL_FILES', GeneratedWebIDLFile),
            ('IPDL_SOURCES', IPDLFile),
            ('LOCAL_INCLUDES', LocalInclude),
            ('PREPROCESSED_WEBIDL_FILES', PreprocessedWebIDLFile),
            ('TEST_WEBIDL_FILES', TestWebIDLFile),
            ('WEBIDL_FILES', WebIDLFile),
            ('XPCSHELL_TESTS_MANIFESTS', XpcshellManifests),
        ]
        for sandbox_var, klass in simple_lists:
            for name in sandbox.get(sandbox_var, []):
                yield klass(sandbox, name)

    def _emit_directory_traversal_from_sandbox(self, sandbox):
        o = DirectoryTraversal(sandbox)
        o.dirs = sandbox.get('DIRS', [])
        o.parallel_dirs = sandbox.get('PARALLEL_DIRS', [])
        o.tool_dirs = sandbox.get('TOOL_DIRS', [])
        o.test_dirs = sandbox.get('TEST_DIRS', [])
        o.test_tool_dirs = sandbox.get('TEST_TOOL_DIRS', [])
        o.external_make_dirs = sandbox.get('EXTERNAL_MAKE_DIRS', [])
        o.parallel_external_make_dirs = sandbox.get('PARALLEL_EXTERNAL_MAKE_DIRS', [])
        o.is_tool_dir = sandbox.get('IS_TOOL_DIR', False)

        if 'TIERS' in sandbox:
            for tier in sandbox['TIERS']:
                o.tier_dirs[tier] = sandbox['TIERS'][tier]['regular'] + \
                    sandbox['TIERS'][tier]['external']
                o.tier_static_dirs[tier] = sandbox['TIERS'][tier]['static']

        yield o
