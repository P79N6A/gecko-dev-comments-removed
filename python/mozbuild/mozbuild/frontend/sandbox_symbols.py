







r"""Defines the global config variables.

This module contains data structures defining the global symbols that have
special meaning in the frontend files for the build system.

If you are looking for the absolute authority on what the global namespace in
the Sandbox consists of, you've come to the right place.
"""

from __future__ import unicode_literals

from collections import OrderedDict
from mozbuild.util import (
    HierarchicalStringList,
    StrictOrderingOnAppendList,
)


def compute_final_target(variables):
    """Convert the default value for FINAL_TARGET"""
    basedir = 'dist/'
    if variables['XPI_NAME']:
        basedir += 'xpi-stage/' + variables['XPI_NAME']
    else:
        basedir += 'bin'
    if variables['DIST_SUBDIR']:
        basedir += '/' + variables['DIST_SUBDIR']
    return basedir
 
 

















VARIABLES = {
    
    'ANDROID_GENERATED_RESFILES': (StrictOrderingOnAppendList, list, [],
        """Android resource files generated as part of the build.

        This variable contains a list of files that are expected to be
        generated (often by preprocessing) into a 'res' directory as
        part of the build process, and subsequently merged into an APK
        file.
        """, 'export'),

    'ANDROID_RESFILES': (StrictOrderingOnAppendList, list, [],
        """Android resource files.

        This variable contains a list of files to package into a 'res'
        directory and merge into an APK file.
        """, 'export'),

    'SOURCES': (StrictOrderingOnAppendList, list, [],
        """Source code files.

        This variable contains a list of source code files to compile.
        Accepts assembler, C, C++, Objective C/C++.
        """, 'compile'),

    'GENERATED_SOURCES': (StrictOrderingOnAppendList, list, [],
        """Generated source code files.

        This variable contains a list of generated source code files to
        compile. Accepts assembler, C, C++, Objective C/C++.
        """, 'compile'),

    'FILES_PER_UNIFIED_FILE': (int, int, None,
        """The number of source files to compile into each unified source file.

        """, 'None'),

    'UNIFIED_SOURCES': (StrictOrderingOnAppendList, list, [],
        """Source code files that can be compiled together.

        This variable contains a list of source code files to compile,
        that can be concatenated all together and built as a single source
        file. This can help make the build faster and reduce the debug info
        size.
        """, 'compile'),

    'GENERATED_UNIFIED_SOURCES': (StrictOrderingOnAppendList, list, [],
        """Generated source code files that can be compiled together.

        This variable contains a list of generated source code files to
        compile, that can be concatenated all together, with UNIFIED_SOURCES,
        and built as a single source file. This can help make the build faster
        and reduce the debug info size.
        """, 'compile'),

    'GENERATED_FILES': (StrictOrderingOnAppendList, list, [],
        """Generic generated files.

        This variable contains a list of generate files for the build system
        to generate at export time. The rules for those files still live in
        Makefile.in.
        """, 'export'),

    'DEFINES': (OrderedDict, dict, OrderedDict(),
        """Dictionary of compiler defines to declare.

        These are passed in to the compiler as ``-Dkey='value'`` for string
        values, ``-Dkey=value`` for numeric values, or ``-Dkey`` if the
        value is True. Note that for string values, the outer-level of
        single-quotes will be consumed by the shell. If you want to have
        a string-literal in the program, the value needs to have
        double-quotes.

        Example::

           DEFINES['NS_NO_XPCOM'] = True
           DEFINES['MOZ_EXTENSIONS_DB_SCHEMA'] = 15
           DEFINES['DLL_SUFFIX'] = '".so"'

        This will result in the compiler flags ``-DNS_NO_XPCOM``,
        ``-DMOZ_EXTENSIONS_DB_SCHEMA=15``, and ``-DDLL_SUFFIX='".so"'``,
        respectively. These could also be combined into a single
        update::

           DEFINES.update({
               'NS_NO_XPCOM': True,
               'MOZ_EXTENSIONS_DB_SCHEMA': 15,
               'DLL_SUFFIX': '".so"',
           })
        """, None),

    'DIRS': (list, list, [],
        """Child directories to descend into looking for build frontend files.

        This works similarly to the ``DIRS`` variable in make files. Each str
        value in the list is the name of a child directory. When this file is
        done parsing, the build reader will descend into each listed directory
        and read the frontend file there. If there is no frontend file, an error
        is raised.

        Values are relative paths. They can be multiple directory levels
        above or below. Use ``..`` for parent directories and ``/`` for path
        delimiters.
        """, None),

    'EXPORT_LIBRARY': (bool, bool, False,
        """Install the library to the static libraries folder.
        """, None),

    'EXTRA_COMPONENTS': (StrictOrderingOnAppendList, list, [],
        """Additional component files to distribute.

       This variable contains a list of files to copy into
       ``$(FINAL_TARGET)/components/``.
        """, 'libs'),

    'EXTRA_JS_MODULES': (StrictOrderingOnAppendList, list, [],
        """Additional JavaScript files to distribute.

        This variable contains a list of files to copy into
        ``$(FINAL_TARGET)/$(JS_MODULES_PATH)``. ``JS_MODULES_PATH`` defaults
        to ``modules`` if left undefined.
        """, 'libs'),

    'EXTRA_PP_JS_MODULES': (StrictOrderingOnAppendList, list, [],
        """Additional JavaScript files to distribute.

        This variable contains a list of files to copy into
        ``$(FINAL_TARGET)/$(JS_MODULES_PATH)``, after preprocessing.
        ``JS_MODULES_PATH`` defaults to ``modules`` if left undefined.
        """, 'libs'),

    'EXTRA_PP_COMPONENTS': (StrictOrderingOnAppendList, list, [],
        """Javascript XPCOM files.

       This variable contains a list of files to preprocess.  Generated
       files will be installed in the ``/components`` directory of the distribution.
        """, 'libs'),

    'FINAL_LIBRARY': (unicode, unicode, "",
        """Library in which the objects of the current directory will be linked.

        This variable contains the name of a library, defined elsewhere with
        ``LIBRARY_NAME``, in which the objects of the current directory will be
        linked.
        """, 'binaries'),

    'CPP_UNIT_TESTS': (StrictOrderingOnAppendList, list, [],
        """C++ source files for unit tests.

        This is a list of C++ unit test sources. Entries must be files that
        exist. These generally have ``.cpp`` extensions.
        """, 'binaries'),

    'FAIL_ON_WARNINGS': (bool, bool, False,
        """Whether to treat warnings as errors.
        """, None),

    'FORCE_SHARED_LIB': (bool, bool, False,
        """Whether the library in this directory is a shared library.
        """, None),

    'FORCE_STATIC_LIB': (bool, bool, False,
        """Whether the library in this directory is a static library.
        """, None),

    'GENERATED_INCLUDES' : (StrictOrderingOnAppendList, list, [],
        """Directories generated by the build system to be searched for include
        files by the compiler.
        """, None),

    'HOST_SOURCES': (StrictOrderingOnAppendList, list, [],
        """Source code files to compile with the host compiler.

        This variable contains a list of source code files to compile.
        with the host compiler.
        """, 'compile'),

    'IS_COMPONENT': (bool, bool, False,
        """Whether the library contains a binary XPCOM component manifest.
        """, None),

    'PARALLEL_DIRS': (list, list, [],
        """A parallel version of ``DIRS``.

        Ideally this variable does not exist. It is provided so a transition
        from recursive makefiles can be made. Once the build system has been
        converted to not use Makefile's for the build frontend, this will
        likely go away.
        """, None),

    'HOST_LIBRARY_NAME': (unicode, unicode, "",
        """Name of target library generated when cross compiling.
        """, 'binaries'),

    'JAVA_JAR_TARGETS': (dict, dict, {},
        """Defines Java JAR targets to be built.

        This variable should not be populated directly. Instead, it should
        populated by calling add_java_jar().
        """, 'binaries'),

    'JS_MODULES_PATH': (unicode, unicode, "",
        """Sub-directory of ``$(FINAL_TARGET)`` to install
        ``EXTRA_JS_MODULES``.

        ``EXTRA_JS_MODULES`` files are copied to
        ``$(FINAL_TARGET)/$(JS_MODULES_PATH)``. This variable does not
        need to be defined if the desired destination directory is
        ``$(FINAL_TARGET)/modules``.
        """, None),

    'LIBRARY_NAME': (unicode, unicode, "",
        """The name of the library generated for a directory.

        In ``example/components/moz.build``,::

           LIBRARY_NAME = 'xpcomsample'

        would generate ``example/components/libxpcomsample.so`` on Linux, or
        ``example/components/xpcomsample.lib`` on Windows.
        """, 'binaries'),

    'LIBS': (StrictOrderingOnAppendList, list, [],
        """Linker libraries and flags.

        A list of libraries and flags to include when linking.
        """, None),

    'LIBXUL_LIBRARY': (bool, bool, False,
        """Whether the library in this directory is linked into libxul.

        Implies ``MOZILLA_INTERNAL_API`` and ``FORCE_STATIC_LIB``.
        """, None),

    'LOCAL_INCLUDES': (StrictOrderingOnAppendList, list, [],
        """Additional directories to be searched for include files by the compiler.
        """, None),

    'MSVC_ENABLE_PGO': (bool, bool, False,
        """Whether profile-guided optimization is enabled in this directory.
        """, None),

    'NO_VISIBILITY_FLAGS': (bool, bool, False,
        """Build sources listed in this file without VISIBILITY_FLAGS.
        """, None),

    'OS_LIBS': (list, list, [],
        """System link libraries.

        This variable contains a list of system libaries to link against.
        """, None),

    'SDK_LIBRARY': (StrictOrderingOnAppendList, list, [],
        """Elements of the distributed SDK.

        Files on this list will be copied into ``SDK_LIB_DIR``
        (``$DIST/sdk/lib``).
        """, None),

    'SIMPLE_PROGRAMS': (StrictOrderingOnAppendList, list, [],
        """Compile a list of executable names.

        Each name in this variable corresponds to an executable built from the
        corresponding source file with the same base name.

        If the configuration token ``BIN_SUFFIX`` is set, its value will be
        automatically appended to each name. If a name already ends with
        ``BIN_SUFFIX``, the name will remain unchanged.
        """, 'binaries'),

    'HOST_SIMPLE_PROGRAMS': (StrictOrderingOnAppendList, list, [],
        """Compile a list of host executable names.

        Each name in this variable corresponds to a hosst executable built
        from the corresponding source file with the same base name.

        If the configuration token ``HOST_BIN_SUFFIX`` is set, its value will
        be automatically appended to each name. If a name already ends with
        ``HOST_BIN_SUFFIX``, the name will remain unchanged.
        """, 'binaries'),

    'TOOL_DIRS': (list, list, [],
        """Like DIRS but for tools.

        Tools are for pieces of the build system that aren't required to
        produce a working binary (in theory). They provide things like test
        code and utilities.
        """, None),

    'TEST_DIRS': (list, list, [],
        """Like DIRS but only for directories that contain test-only code.

        If tests are not enabled, this variable will be ignored.

        This variable may go away once the transition away from Makefiles is
        complete.
        """, None),

    'TEST_TOOL_DIRS': (list, list, [],
        """TOOL_DIRS that is only executed if tests are enabled.
        """, None),


    'TIERS': (OrderedDict, dict, OrderedDict(),
        """Defines directories constituting the tier traversal mechanism.

        The recursive make backend iteration is organized into tiers. There are
        major tiers (keys in this dict) that correspond roughly to applications
        or libraries being built. e.g. base, nspr, js, platform, app. Within
        each tier are phases like export, libs, and tools. The recursive make
        backend iterates over each phase in the first tier then proceeds to the
        next tier until all tiers are exhausted.

        Tiers are a way of working around deficiencies in recursive make. These
        will probably disappear once we no longer rely on recursive make for
        the build backend. They will likely be replaced by ``DIRS``.

        This variable is typically not populated directly. Instead, it is
        populated by calling add_tier_dir().
        """, None),

    'EXTERNAL_MAKE_DIRS': (list, list, [],
        """Directories that build with make but don't use moz.build files.

        This is like ``DIRS`` except it implies that ``make`` is used to build the
        directory and that the directory does not define itself with moz.build
        files.
        """, None),

    'PARALLEL_EXTERNAL_MAKE_DIRS': (list, list, [],
        """Parallel version of ``EXTERNAL_MAKE_DIRS``.
        """, None),

    'CONFIGURE_SUBST_FILES': (StrictOrderingOnAppendList, list, [],
        """Output files that will be generated using configure-like substitution.

        This is a substitute for ``AC_OUTPUT`` in autoconf. For each path in this
        list, we will search for a file in the srcdir having the name
        ``{path}.in``. The contents of this file will be read and variable
        patterns like ``@foo@`` will be substituted with the values of the
        ``AC_SUBST`` variables declared during configure.
        """, None),

    'CONFIGURE_DEFINE_FILES': (StrictOrderingOnAppendList, list, [],
        """Output files generated from configure/config.status.

        This is a substitute for ``AC_CONFIG_HEADER`` in autoconf. This is very
        similar to ``CONFIGURE_SUBST_FILES`` except the generation logic takes
        into account the values of ``AC_DEFINE`` instead of ``AC_SUBST``.
        """, None),

    'EXPORTS': (HierarchicalStringList, list, HierarchicalStringList(),
        """List of files to be exported, and in which subdirectories.

        ``EXPORTS`` is generally used to list the include files to be exported to
        ``dist/include``, but it can be used for other files as well. This variable
        behaves as a list when appending filenames for export in the top-level
        directory. Files can also be appended to a field to indicate which
        subdirectory they should be exported to. For example, to export
        ``foo.h`` to the top-level directory, and ``bar.h`` to ``mozilla/dom/``,
        append to ``EXPORTS`` like so::

           EXPORTS += ['foo.h']
           EXPORTS.mozilla.dom += ['bar.h']
        """, None),

    'PROGRAM' : (unicode, unicode, "",
        """Compiled executable name.

        If the configuration token ``BIN_SUFFIX`` is set, its value will be
        automatically appended to ``PROGRAM``. If ``PROGRAM`` already ends with
        ``BIN_SUFFIX``, ``PROGRAM`` will remain unchanged.
        """, 'binaries'),

    'HOST_PROGRAM' : (unicode, unicode, "",
        """Compiled host executable name.

        If the configuration token ``HOST_BIN_SUFFIX`` is set, its value will be
        automatically appended to ``HOST_PROGRAM``. If ``HOST_PROGRAM`` already
        ends with ``HOST_BIN_SUFFIX``, ``HOST_PROGRAM`` will remain unchanged.
        """, 'binaries'),

    'NO_DIST_INSTALL': (bool, bool, False,
        """Disable installing certain files into the distribution directory.

        If present, some files defined by other variables won't be
        distributed/shipped with the produced build.
        """, None),

    
    'XPIDL_SOURCES': (StrictOrderingOnAppendList, list, [],
        """XPCOM Interface Definition Files (xpidl).

        This is a list of files that define XPCOM interface definitions.
        Entries must be files that exist. Entries are almost certainly ``.idl``
        files.
        """, 'libs'),

    'XPIDL_MODULE': (unicode, unicode, "",
        """XPCOM Interface Definition Module Name.

        This is the name of the ``.xpt`` file that is created by linking
        ``XPIDL_SOURCES`` together. If unspecified, it defaults to be the same
        as ``MODULE``.
        """, None),

    'IPDL_SOURCES': (StrictOrderingOnAppendList, list, [],
        """IPDL source files.

        These are ``.ipdl`` files that will be parsed and converted to
        ``.cpp`` files.
        """, 'export'),

    'WEBIDL_FILES': (StrictOrderingOnAppendList, list, [],
        """WebIDL source files.

        These will be parsed and converted to ``.cpp`` and ``.h`` files.
        """, 'export'),

    'GENERATED_EVENTS_WEBIDL_FILES': (StrictOrderingOnAppendList, list, [],
        """WebIDL source files for generated events.

        These will be parsed and converted to ``.cpp`` and ``.h`` files.
        """, 'export'),

    'TEST_WEBIDL_FILES': (StrictOrderingOnAppendList, list, [],
         """Test WebIDL source files.

         These will be parsed and converted to ``.cpp`` and ``.h`` files
         if tests are enabled.
         """, 'export'),

    'GENERATED_WEBIDL_FILES': (StrictOrderingOnAppendList, list, [],
         """Generated WebIDL source files.

         These will be generated from some other files.
         """, 'export'),

    'PREPROCESSED_TEST_WEBIDL_FILES': (StrictOrderingOnAppendList, list, [],
         """Preprocessed test WebIDL source files.

         These will be preprocessed, then parsed and converted to .cpp
         and ``.h`` files if tests are enabled.
         """, 'export'),

    'PREPROCESSED_WEBIDL_FILES': (StrictOrderingOnAppendList, list, [],
         """Preprocessed WebIDL source files.

         These will be preprocessed before being parsed and converted.
         """, 'export'),

    
    'A11Y_MANIFESTS': (StrictOrderingOnAppendList, list, [],
        """List of manifest files defining a11y tests.
        """, None),

    'BROWSER_CHROME_MANIFESTS': (StrictOrderingOnAppendList, list, [],
        """List of manifest files defining browser chrome tests.
        """, None),

    'METRO_CHROME_MANIFESTS': (StrictOrderingOnAppendList, list, [],
        """List of manifest files defining metro browser chrome tests.
        """, None),

    'MOCHITEST_CHROME_MANIFESTS': (StrictOrderingOnAppendList, list, [],
        """List of manifest files defining mochitest chrome tests.
        """, None),

    'MOCHITEST_MANIFESTS': (StrictOrderingOnAppendList, list, [],
        """List of manifest files defining mochitest tests.
        """, None),

    'MOCHITEST_WEBAPPRT_CHROME_MANIFESTS': (StrictOrderingOnAppendList, list, [],
        """List of manifest files defining webapprt mochitest chrome tests.
        """, None),

    'WEBRTC_SIGNALLING_TEST_MANIFESTS': (StrictOrderingOnAppendList, list, [],
        """List of manifest files defining WebRTC signalling tests.
        """, None),

    'XPCSHELL_TESTS_MANIFESTS': (StrictOrderingOnAppendList, list, [],
        """List of manifest files defining xpcshell tests.
        """, None),

    
    'XPI_NAME': (unicode, unicode, "",
        """The name of an extension XPI to generate.

        When this variable is present, the results of this directory will end up
        being packaged into an extension instead of the main dist/bin results.
        """, 'libs'),

    'DIST_SUBDIR': (unicode, unicode, "",
        """The name of an alternate directory to install files to.

        When this variable is present, the results of this directory will end up
        being placed in the $(DIST_SUBDIR) subdirectory of where it would
        otherwise be placed.
        """, 'libs'),

    'FINAL_TARGET': (unicode, unicode, compute_final_target,
        """The name of the directory to install targets to.

        The directory is relative to the top of the object directory. The
        default value is dependent on the values of XPI_NAME and DIST_SUBDIR. If
        neither are present, the result is dist/bin. If XPI_NAME is present, the
        result is dist/xpi-stage/$(XPI_NAME). If DIST_SUBDIR is present, then
        the $(DIST_SUBDIR) directory of the otherwise default value is used.
        """, 'libs'),
}









FUNCTIONS = {
    'include': ('_include', (str,),
        """Include another mozbuild file in the context of this one.

        This is similar to a ``#include`` in C languages. The filename passed to
        the function will be read and its contents will be evaluated within the
        context of the calling file.

        If a relative path is given, it is evaluated as relative to the file
        currently being processed. If there is a chain of multiple include(),
        the relative path computation is from the most recent/active file.

        If an absolute path is given, it is evaluated from ``TOPSRCDIR``. In
        other words, ``include('/foo')`` references the path
        ``TOPSRCDIR + '/foo'``.

        Example usage
        ^^^^^^^^^^^^^

        Include ``sibling.build`` from the current directory.::

           include('sibling.build')

        Include ``foo.build`` from a path within the top source directory::

           include('/elsewhere/foo.build')
        """),

    'add_java_jar': ('_add_java_jar', (str,),
        """Declare a Java JAR target to be built.

        This is the supported way to populate the JAVA_JAR_TARGETS
        variable.

        The parameters are:
        * dest - target name, without the trailing .jar. (required)

        This returns a rich Java JAR type, described at
        :py:class:`mozbuild.frontend.data.JavaJarData`.
        """),

    'add_tier_dir': ('_add_tier_directory', (str, [str, list], bool, bool),
        """Register a directory for tier traversal.

        This is the preferred way to populate the TIERS variable.

        Tiers are how the build system is organized. The build process is
        divided into major phases called tiers. The most important tiers are
        "platform" and "apps." The platform tier builds the Gecko platform
        (typically outputting libxul). The apps tier builds the configured
        application (browser, mobile/android, b2g, etc).

        This function is typically only called by the main moz.build file or a
        file directly included by the main moz.build file. An error will be
        raised if it is called when it shouldn't be.

        An error will also occur if you attempt to add the same directory to
        the same tier multiple times.

        Example usage
        ^^^^^^^^^^^^^

        Register a single directory with the 'platform' tier::

           add_tier_dir('platform', 'xul')

        Register multiple directories with the 'app' tier.::

           add_tier_dir('app', ['components', 'base'])

        Register a directory as having static content (no dependencies)::

           add_tier_dir('base', 'foo', static=True)

        Register a directory as having external content (same as static
        content, but traversed with export, libs, and tools subtiers::

           add_tier_dir('base', 'bar', external=True)
        """),

    'export': ('_export', (str,),
        """Make the specified variable available to all child directories.

        The variable specified by the argument string is added to the
        environment of all directories specified in the DIRS, PARALLEL_DIRS,
        TOOL_DIRS, TEST_DIRS, and TEST_TOOL_DIRS variables. If those directories
        themselves have child directories, the variable will be exported to all
        of them.

        The value used for the variable is the final value at the end of the
        moz.build file, so it is possible (but not recommended style) to place
        the export before the definition of the variable.

        This function is limited to the upper-case variables that have special
        meaning in moz.build files.

        NOTE: Please consult with a build peer before adding a new use of this
        function.

        Example usage
        ^^^^^^^^^^^^^

        To make all children directories install as the given extension::

          XPI_NAME = 'cool-extension'
          export('XPI_NAME')
        """),

    'warning': ('_warning', (str,),
        """Issue a warning.

        Warnings are string messages that are printed during execution.

        Warnings are ignored during execution.
        """),

    'error': ('_error', (str,),
        """Issue a fatal error.

        If this function is called, processing is aborted immediately.
        """),
}


SPECIAL_VARIABLES = {
    'TOPSRCDIR': (str,
        """Constant defining the top source directory.

        The top source directory is the parent directory containing the source
        code and all build files. It is typically the root directory of a
        cloned repository.
        """),

    'TOPOBJDIR': (str,
        """Constant defining the top object directory.

        The top object directory is the parent directory which will contain
        the output of the build. This is commonly referred to as "the object
        directory."
        """),

    'RELATIVEDIR': (str,
        """Constant defining the relative path of this file.

        The relative path is from ``TOPSRCDIR``. This is defined as relative
        to the main file being executed, regardless of whether additional
        files have been included using ``include()``.
        """),

    'SRCDIR': (str,
        """Constant defining the source directory of this file.

        This is the path inside ``TOPSRCDIR`` where this file is located. It
        is the same as ``TOPSRCDIR + RELATIVEDIR``.
        """),

    'OBJDIR': (str,
        """The path to the object directory for this file.

        Is is the same as ``TOPOBJDIR + RELATIVEDIR``.
        """),

    'CONFIG': (dict,
        """Dictionary containing the current configuration variables.

        All the variables defined by the configuration system are available
        through this object. e.g. ``ENABLE_TESTS``, ``CFLAGS``, etc.

        Values in this container are read-only. Attempts at changing values
        will result in a run-time error.

        Access to an unknown variable will return None.
        """),

    '__builtins__': (dict,
        """Exposes Python built-in types.

        The set of exposed Python built-ins is currently:

        - True
        - False
        - None
        """),
}
