



"""
This module contains classes that help to emulate xcodebuild behavior on top of
other build systems, such as make and ninja.
"""

import gyp.common
import os.path
import re
import shlex

class XcodeSettings(object):
  """A class that understands the gyp 'xcode_settings' object."""

  def __init__(self, spec):
    self.spec = spec

    
    
    
    
    self.xcode_settings = {}
    configs = spec['configurations']
    for configname, config in configs.iteritems():
      self.xcode_settings[configname] = config.get('xcode_settings', {})

    
    self.configname = None

    
    self.library_re = re.compile(r'^lib([^/]+)\.(a|dylib)$')

  def _Settings(self):
    assert self.configname
    return self.xcode_settings[self.configname]

  def _Test(self, test_key, cond_key, default):
    return self._Settings().get(test_key, default) == cond_key

  def _Appendf(self, lst, test_key, format_str, default=None):
    if test_key in self._Settings():
      lst.append(format_str % str(self._Settings()[test_key]))
    elif default:
      lst.append(format_str % str(default))

  def _WarnUnimplemented(self, test_key):
    if test_key in self._Settings():
      print 'Warning: Ignoring not yet implemented key "%s".' % test_key

  def _IsBundle(self):
    return int(self.spec.get('mac_bundle', 0)) != 0

  def GetFrameworkVersion(self):
    """Returns the framework version of the current target. Only valid for
    bundles."""
    assert self._IsBundle()
    return self.GetPerTargetSetting('FRAMEWORK_VERSION', default='A')

  def GetWrapperExtension(self):
    """Returns the bundle extension (.app, .framework, .plugin, etc).  Only
    valid for bundles."""
    assert self._IsBundle()
    if self.spec['type'] in ('loadable_module', 'shared_library'):
      default_wrapper_extension = {
        'loadable_module': 'bundle',
        'shared_library': 'framework',
      }[self.spec['type']]
      wrapper_extension = self.GetPerTargetSetting(
          'WRAPPER_EXTENSION', default=default_wrapper_extension)
      return '.' + self.spec.get('product_extension', wrapper_extension)
    elif self.spec['type'] == 'executable':
      return '.app'
    else:
      assert False, "Don't know extension for '%s', target '%s'" % (
          self.spec['type'], self.spec['target_name'])

  def GetProductName(self):
    """Returns PRODUCT_NAME."""
    return self.spec.get('product_name', self.spec['target_name'])

  def GetFullProductName(self):
    """Returns FULL_PRODUCT_NAME."""
    if self._IsBundle():
      return self.GetWrapperName()
    else:
      return self._GetStandaloneBinaryPath()

  def GetWrapperName(self):
    """Returns the directory name of the bundle represented by this target.
    Only valid for bundles."""
    assert self._IsBundle()
    return self.GetProductName() + self.GetWrapperExtension()

  def GetBundleContentsFolderPath(self):
    """Returns the qualified path to the bundle's contents folder. E.g.
    Chromium.app/Contents or Foo.bundle/Versions/A. Only valid for bundles."""
    assert self._IsBundle()
    if self.spec['type'] == 'shared_library':
      return os.path.join(
          self.GetWrapperName(), 'Versions', self.GetFrameworkVersion())
    else:
      
      return os.path.join(self.GetWrapperName(), 'Contents')

  def GetBundleResourceFolder(self):
    """Returns the qualified path to the bundle's resource folder. E.g.
    Chromium.app/Contents/Resources. Only valid for bundles."""
    assert self._IsBundle()
    return os.path.join(self.GetBundleContentsFolderPath(), 'Resources')

  def GetBundlePlistPath(self):
    """Returns the qualified path to the bundle's plist file. E.g.
    Chromium.app/Contents/Info.plist. Only valid for bundles."""
    assert self._IsBundle()
    if self.spec['type'] in ('executable', 'loadable_module'):
      return os.path.join(self.GetBundleContentsFolderPath(), 'Info.plist')
    else:
      return os.path.join(self.GetBundleContentsFolderPath(),
                          'Resources', 'Info.plist')

  def GetProductType(self):
    """Returns the PRODUCT_TYPE of this target."""
    if self._IsBundle():
      return {
        'executable': 'com.apple.product-type.application',
        'loadable_module': 'com.apple.product-type.bundle',
        'shared_library': 'com.apple.product-type.framework',
      }[self.spec['type']]
    else:
      return {
        'executable': 'com.apple.product-type.tool',
        'loadable_module': 'com.apple.product-type.library.dynamic',
        'shared_library': 'com.apple.product-type.library.dynamic',
        'static_library': 'com.apple.product-type.library.static',
      }[self.spec['type']]

  def GetMachOType(self):
    """Returns the MACH_O_TYPE of this target."""
    
    if not self._IsBundle() and self.spec['type'] == 'executable':
      return ''
    return {
      'executable': 'mh_execute',
      'static_library': 'staticlib',
      'shared_library': 'mh_dylib',
      'loadable_module': 'mh_bundle',
    }[self.spec['type']]

  def _GetBundleBinaryPath(self):
    """Returns the name of the bundle binary of by this target.
    E.g. Chromium.app/Contents/MacOS/Chromium. Only valid for bundles."""
    assert self._IsBundle()
    if self.spec['type'] in ('shared_library'):
      path = self.GetBundleContentsFolderPath()
    elif self.spec['type'] in ('executable', 'loadable_module'):
      path = os.path.join(self.GetBundleContentsFolderPath(), 'MacOS')
    return os.path.join(path, self.GetExecutableName())

  def _GetStandaloneExecutableSuffix(self):
    if 'product_extension' in self.spec:
      return '.' + self.spec['product_extension']
    return {
      'executable': '',
      'static_library': '.a',
      'shared_library': '.dylib',
      'loadable_module': '.so',
    }[self.spec['type']]

  def _GetStandaloneExecutablePrefix(self):
    return self.spec.get('product_prefix', {
      'executable': '',
      'static_library': 'lib',
      'shared_library': 'lib',
      
      
      'loadable_module': '',
    }[self.spec['type']])

  def _GetStandaloneBinaryPath(self):
    """Returns the name of the non-bundle binary represented by this target.
    E.g. hello_world. Only valid for non-bundles."""
    assert not self._IsBundle()
    assert self.spec['type'] in (
        'executable', 'shared_library', 'static_library', 'loadable_module'), (
        'Unexpected type %s' % self.spec['type'])
    target = self.spec['target_name']
    if self.spec['type'] == 'static_library':
      if target[:3] == 'lib':
        target = target[3:]
    elif self.spec['type'] in ('loadable_module', 'shared_library'):
      if target[:3] == 'lib':
        target = target[3:]

    target_prefix = self._GetStandaloneExecutablePrefix()
    target = self.spec.get('product_name', target)
    target_ext = self._GetStandaloneExecutableSuffix()
    return target_prefix + target + target_ext

  def GetExecutableName(self):
    """Returns the executable name of the bundle represented by this target.
    E.g. Chromium."""
    if self._IsBundle():
      return self.spec.get('product_name', self.spec['target_name'])
    else:
      return self._GetStandaloneBinaryPath()

  def GetExecutablePath(self):
    """Returns the directory name of the bundle represented by this target. E.g.
    Chromium.app/Contents/MacOS/Chromium."""
    if self._IsBundle():
      return self._GetBundleBinaryPath()
    else:
      return self._GetStandaloneBinaryPath()

  def _SdkPath(self):
    sdk_root = self.GetPerTargetSetting('SDKROOT', default='macosx10.5')
    if sdk_root.startswith('macosx'):
      sdk_root = 'MacOSX' + sdk_root[len('macosx'):]
    return '/Developer/SDKs/%s.sdk' % sdk_root

  def GetCflags(self, configname):
    """Returns flags that need to be added to .c, .cc, .m, and .mm
    compilations."""
    
    

    self.configname = configname
    cflags = []

    sdk_root = self._SdkPath()
    if 'SDKROOT' in self._Settings():
      cflags.append('-isysroot %s' % sdk_root)

    if self._Test('GCC_CW_ASM_SYNTAX', 'YES', default='YES'):
      cflags.append('-fasm-blocks')

    if 'GCC_DYNAMIC_NO_PIC' in self._Settings():
      if self._Settings()['GCC_DYNAMIC_NO_PIC'] == 'YES':
        cflags.append('-mdynamic-no-pic')
    else:
      pass
      
      
      

    if self._Test('GCC_ENABLE_PASCAL_STRINGS', 'YES', default='YES'):
      cflags.append('-mpascal-strings')

    self._Appendf(cflags, 'GCC_OPTIMIZATION_LEVEL', '-O%s', default='s')

    if self._Test('GCC_GENERATE_DEBUGGING_SYMBOLS', 'YES', default='YES'):
      dbg_format = self._Settings().get('DEBUG_INFORMATION_FORMAT', 'dwarf')
      if dbg_format == 'dwarf':
        cflags.append('-gdwarf-2')
      elif dbg_format == 'stabs':
        raise NotImplementedError('stabs debug format is not supported yet.')
      elif dbg_format == 'dwarf-with-dsym':
        cflags.append('-gdwarf-2')
      else:
        raise NotImplementedError('Unknown debug format %s' % dbg_format)

    if self._Test('GCC_SYMBOLS_PRIVATE_EXTERN', 'YES', default='NO'):
      cflags.append('-fvisibility=hidden')

    if self._Test('GCC_TREAT_WARNINGS_AS_ERRORS', 'YES', default='NO'):
      cflags.append('-Werror')

    if self._Test('GCC_WARN_ABOUT_MISSING_NEWLINE', 'YES', default='NO'):
      cflags.append('-Wnewline-eof')

    self._Appendf(cflags, 'MACOSX_DEPLOYMENT_TARGET', '-mmacosx-version-min=%s')

    
    self._WarnUnimplemented('ARCHS')
    if self._Test('COPY_PHASE_STRIP', 'YES', default='NO'):
      self._WarnUnimplemented('COPY_PHASE_STRIP')
    self._WarnUnimplemented('GCC_DEBUGGING_SYMBOLS')
    self._WarnUnimplemented('GCC_ENABLE_OBJC_EXCEPTIONS')
    self._WarnUnimplemented('GCC_ENABLE_OBJC_GC')

    
    self._WarnUnimplemented('MACH_O_TYPE')
    self._WarnUnimplemented('PRODUCT_TYPE')

    
    
    
    cflags.append('-arch i386')

    cflags += self._Settings().get('OTHER_CFLAGS', [])
    cflags += self._Settings().get('WARNING_CFLAGS', [])

    config = self.spec['configurations'][self.configname]
    framework_dirs = config.get('mac_framework_dirs', [])
    for directory in framework_dirs:
      cflags.append('-F ' + directory.replace('$(SDKROOT)', sdk_root))

    self.configname = None
    return cflags

  def GetCflagsC(self, configname):
    """Returns flags that need to be added to .c, and .m compilations."""
    self.configname = configname
    cflags_c = []
    self._Appendf(cflags_c, 'GCC_C_LANGUAGE_STANDARD', '-std=%s')
    self.configname = None
    return cflags_c

  def GetCflagsCC(self, configname):
    """Returns flags that need to be added to .cc, and .mm compilations."""
    self.configname = configname
    cflags_cc = []
    if self._Test('GCC_ENABLE_CPP_RTTI', 'NO', default='YES'):
      cflags_cc.append('-fno-rtti')
    if self._Test('GCC_ENABLE_CPP_EXCEPTIONS', 'NO', default='YES'):
      cflags_cc.append('-fno-exceptions')
    if self._Test('GCC_INLINES_ARE_PRIVATE_EXTERN', 'YES', default='NO'):
      cflags_cc.append('-fvisibility-inlines-hidden')
    if self._Test('GCC_THREADSAFE_STATICS', 'NO', default='YES'):
      cflags_cc.append('-fno-threadsafe-statics')
    self.configname = None
    return cflags_cc

  def GetCflagsObjC(self, configname):
    """Returns flags that need to be added to .m compilations."""
    self.configname = configname
    self.configname = None
    return []

  def GetCflagsObjCC(self, configname):
    """Returns flags that need to be added to .mm compilations."""
    self.configname = configname
    cflags_objcc = []
    if self._Test('GCC_OBJC_CALL_CXX_CDTORS', 'YES', default='NO'):
      cflags_objcc.append('-fobjc-call-cxx-cdtors')
    self.configname = None
    return cflags_objcc

  def GetLdflags(self, configname, product_dir, gyp_to_build_path):
    """Returns flags that need to be passed to the linker.

    Args:
        configname: The name of the configuration to get ld flags for.
        product_dir: The directory where products such static and dynamic
            libraries are placed. This is added to the library search path.
        gyp_to_build_path: A function that converts paths relative to the
            current gyp file to paths relative to the build direcotry.
    """
    self.configname = configname
    ldflags = []

    
    
    
    def MapGypPathWithPrefix(flag, prefix):
      if flag.startswith(prefix):
        flag = prefix + gyp_to_build_path(flag[len(prefix):])
      return flag
    for ldflag in self._Settings().get('OTHER_LDFLAGS', []):
      
      
      ldflag = MapGypPathWithPrefix(ldflag, '-L')
      
      ldflag = MapGypPathWithPrefix(ldflag, '-Wl,-exported_symbols_list ')
      ldflags.append(ldflag)

    if self._Test('DEAD_CODE_STRIPPING', 'YES', default='NO'):
      ldflags.append('-Wl,-dead_strip')

    if self._Test('PREBINDING', 'YES', default='NO'):
      ldflags.append('-Wl,-prebind')

    self._Appendf(
        ldflags, 'DYLIB_COMPATIBILITY_VERSION', '-compatibility_version %s')
    self._Appendf(
        ldflags, 'DYLIB_CURRENT_VERSION', '-current_version %s')
    self._Appendf(
        ldflags, 'MACOSX_DEPLOYMENT_TARGET', '-mmacosx-version-min=%s')
    if 'SDKROOT' in self._Settings():
      ldflags.append('-isysroot ' + self._SdkPath())

    for library_path in self._Settings().get('LIBRARY_SEARCH_PATHS', []):
      ldflags.append('-L' + gyp_to_build_path(library_path))

    if 'ORDER_FILE' in self._Settings():
      ldflags.append('-Wl,-order_file ' +
                     '-Wl,' + gyp_to_build_path(
                                  self._Settings()['ORDER_FILE']))

    
    ldflags.append('-arch i386')

    
    ldflags.append('-L' + product_dir)

    install_name = self.GetPerTargetSetting('LD_DYLIB_INSTALL_NAME')
    install_base = self.GetPerTargetSetting('DYLIB_INSTALL_NAME_BASE')
    default_install_name = \
          '$(DYLIB_INSTALL_NAME_BASE:standardizepath)/$(EXECUTABLE_PATH)'
    if not install_name and install_base:
      install_name = default_install_name

    if install_name:
      
      
      if '$' in install_name:
        assert install_name in ('$(DYLIB_INSTALL_NAME_BASE:standardizepath)/'
            '$(WRAPPER_NAME)/$(PRODUCT_NAME)', default_install_name), (
            'Variables in LD_DYLIB_INSTALL_NAME are not generally supported yet'
            ' in target \'%s\' (got \'%s\')' %
                (self.spec['target_name'], install_name))
        
        
        if '/' in install_base:
          prefix, rest = '', install_base
          if install_base.startswith('@'):
            prefix, rest = install_base.split('/', 1)
          rest = os.path.normpath(rest)  
          install_base = os.path.join(prefix, rest)

        install_name = install_name.replace(
            '$(DYLIB_INSTALL_NAME_BASE:standardizepath)', install_base)
        if self._IsBundle():
          
          install_name = install_name.replace(
              '$(WRAPPER_NAME)', self.GetWrapperName())
          install_name = install_name.replace(
              '$(PRODUCT_NAME)', self.GetProductName())
        else:
          assert '$(WRAPPER_NAME)' not in install_name
          assert '$(PRODUCT_NAME)' not in install_name

        install_name = install_name.replace(
            '$(EXECUTABLE_PATH)', self.GetExecutablePath())

      install_name = install_name.replace(' ', r'\ ')
      ldflags.append('-install_name ' + install_name)

    self.configname = None
    return ldflags

  def GetPerTargetSettings(self):
    """Gets a list of all the per-target settings. This will only fetch keys
    whose values are the same across all configurations."""
    first_pass = True
    result = {}
    for configname in sorted(self.xcode_settings.keys()):
      if first_pass:
        result = dict(self.xcode_settings[configname])
        first_pass = False
      else:
        for key, value in self.xcode_settings[configname].iteritems():
          if key not in result:
            continue
          elif result[key] != value:
            del result[key]
    return result

  def GetPerTargetSetting(self, setting, default=None):
    """Tries to get xcode_settings.setting from spec. Assumes that the setting
       has the same value in all configurations and throws otherwise."""
    first_pass = True
    result = None
    for configname in sorted(self.xcode_settings.keys()):
      if first_pass:
        result = self.xcode_settings[configname].get(setting, None)
        first_pass = False
      else:
        assert result == self.xcode_settings[configname].get(setting, None), (
            "Expected per-target setting for '%s', got per-config setting "
            "(target %s)" % (setting, spec['target_name']))
    if result is None:
      return default
    return result

  def _GetStripPostbuilds(self, configname, output_binary):
    """Returns a list of shell commands that contain the shell commands
    neccessary to strip this target's binary. These should be run as postbuilds
    before the actual postbuilds run."""
    self.configname = configname

    result = []
    if (self._Test('DEPLOYMENT_POSTPROCESSING', 'YES', default='NO') and
        self._Test('STRIP_INSTALLED_PRODUCT', 'YES', default='NO')):

      default_strip_style = 'debugging'
      if self._IsBundle():
        default_strip_style = 'non-global'
      elif self.spec['type'] == 'executable':
        default_strip_style = 'all'

      strip_style = self._Settings().get('STRIP_STYLE', default_strip_style)
      strip_flags = {
        'all': '',
        'non-global': '-x',
        'debugging': '-S',
      }[strip_style]

      explicit_strip_flags = self._Settings().get('STRIPFLAGS', '')
      if explicit_strip_flags:
        strip_flags += ' ' + _NormalizeEnvVarReferences(explicit_strip_flags)

      result.append('echo STRIP\\(%s\\)' % self.spec['target_name'])
      result.append('strip %s %s' % (strip_flags, output_binary))

    self.configname = None
    return result

  def _GetDebugInfoPostbuilds(self, configname, output, output_binary):
    """Returns a list of shell commands that contain the shell commands
    neccessary to massage this target's debug information. These should be run
    as postbuilds before the actual postbuilds run."""
    self.configname = configname

    
    result = []
    if (self._Test('GCC_GENERATE_DEBUGGING_SYMBOLS', 'YES', default='YES') and
        self._Test(
            'DEBUG_INFORMATION_FORMAT', 'dwarf-with-dsym', default='dwarf') and
        self.spec['type'] != 'static_library'):
      result.append('echo DSYMUTIL\\(%s\\)' % self.spec['target_name'])
      result.append('dsymutil %s -o %s' % (output_binary, output + '.dSYM'))

    self.configname = None
    return result

  def GetTargetPostbuilds(self, configname, output, output_binary):
    """Returns a list of shell commands that contain the shell commands
    to run as postbuilds for this target, before the actual postbuilds."""
    
    return (self._GetDebugInfoPostbuilds(configname, output, output_binary) +
            self._GetStripPostbuilds(configname, output_binary))

  def _AdjustLibrary(self, library):
    if library.endswith('.framework'):
      l = '-framework ' + os.path.splitext(os.path.basename(library))[0]
    else:
      m = self.library_re.match(library)
      if m:
        l = '-l' + m.group(1)
      else:
        l = library
    return l.replace('$(SDKROOT)', self._SdkPath())

  def AdjustLibraries(self, libraries):
    """Transforms entries like 'Cocoa.framework' in libraries into entries like
    '-framework Cocoa', 'libcrypto.dylib' into '-lcrypto', etc.
    """
    libraries = [ self._AdjustLibrary(library) for library in libraries]
    return libraries


class MacPrefixHeader(object):
  """A class that helps with emulating Xcode's GCC_PREFIX_HEADER feature.

  This feature consists of several pieces:
  * If GCC_PREFIX_HEADER is present, all compilations in that project get an
    additional |-include path_to_prefix_header| cflag.
  * If GCC_PRECOMPILE_PREFIX_HEADER is present too, then the prefix header is
    instead compiled, and all other compilations in the project get an
    additional |-include path_to_compiled_header| instead.
    + Compiled prefix headers have the extension gch. There is one gch file for
      every language used in the project (c, cc, m, mm), since gch files for
      different languages aren't compatible.
    + gch files themselves are built with the target's normal cflags, but they
      obviously don't get the |-include| flag. Instead, they need a -x flag that
      describes their language.
    + All o files in the target need to depend on the gch file, to make sure
      it's built before any o file is built.

  This class helps with some of these tasks, but it needs help from the build
  system for writing dependencies to the gch files, for writing build commands
  for the gch files, and for figuring out the location of the gch files.
  """
  def __init__(self, xcode_settings,
               gyp_path_to_build_path, gyp_path_to_build_output):
    """If xcode_settings is None, all methods on this class are no-ops.

    Args:
        gyp_path_to_build_path: A function that takes a gyp-relative path,
            and returns a path relative to the build directory.
        gyp_path_to_build_output: A function that takes a gyp-relative path and
            a language code ('c', 'cc', 'm', or 'mm'), and that returns a path
            to where the output of precompiling that path for that language
            should be placed (without the trailing '.gch').
    """
    
    
    self.header = None
    self.compile_headers = False
    if xcode_settings:
      self.header = xcode_settings.GetPerTargetSetting('GCC_PREFIX_HEADER')
      self.compile_headers = xcode_settings.GetPerTargetSetting(
          'GCC_PRECOMPILE_PREFIX_HEADER', default='NO') != 'NO'
    self.compiled_headers = {}
    if self.header:
      if self.compile_headers:
        for lang in ['c', 'cc', 'm', 'mm']:
          self.compiled_headers[lang] = gyp_path_to_build_output(
              self.header, lang)
      self.header = gyp_path_to_build_path(self.header)

  def GetInclude(self, lang):
    """Gets the cflags to include the prefix header for language |lang|."""
    if self.compile_headers and lang in self.compiled_headers:
      return '-include %s' % self.compiled_headers[lang]
    elif self.header:
      return '-include %s' % self.header
    else:
      return ''

  def _Gch(self, lang):
    """Returns the actual file name of the prefix header for language |lang|."""
    assert self.compile_headers
    return self.compiled_headers[lang] + '.gch'

  def GetObjDependencies(self, sources, objs):
    """Given a list of source files and the corresponding object files, returns
    a list of (source, object, gch) tuples, where |gch| is the build-directory
    relative path to the gch file each object file depends on.  |compilable[i]|
    has to be the source file belonging to |objs[i]|."""
    if not self.header or not self.compile_headers:
      return []

    result = []
    for source, obj in zip(sources, objs):
      ext = os.path.splitext(source)[1]
      lang = {
        '.c': 'c',
        '.cpp': 'cc', '.cc': 'cc', '.cxx': 'cc',
        '.m': 'm',
        '.mm': 'mm',
      }.get(ext, None)
      if lang:
        result.append((source, obj, self._Gch(lang)))
    return result

  def GetGchBuildCommands(self):
    """Returns [(path_to_gch, language_flag, language, header)].
    |path_to_gch| and |header| are relative to the build directory.
    """
    if not self.header or not self.compile_headers:
      return []
    return [
      (self._Gch('c'), '-x c-header', 'c', self.header),
      (self._Gch('cc'), '-x c++-header', 'cc', self.header),
      (self._Gch('m'), '-x objective-c-header', 'm', self.header),
      (self._Gch('mm'), '-x objective-c++-header', 'mm', self.header),
    ]


def MergeGlobalXcodeSettingsToSpec(global_dict, spec):
  """Merges the global xcode_settings dictionary into each configuration of the
  target represented by spec. For keys that are both in the global and the local
  xcode_settings dict, the local key gets precendence.
  """
  
  
  
  global_xcode_settings = global_dict.get('xcode_settings', {})
  for config in spec['configurations'].values():
    if 'xcode_settings' in config:
      new_settings = global_xcode_settings.copy()
      new_settings.update(config['xcode_settings'])
      config['xcode_settings'] = new_settings


def IsMacBundle(flavor, spec):
  """Returns if |spec| should be treated as a bundle.

  Bundles are directories with a certain subdirectory structure, instead of
  just a single file. Bundle rules do not produce a binary but also package
  resources into that directory."""
  is_mac_bundle = (int(spec.get('mac_bundle', 0)) != 0 and flavor == 'mac')
  if is_mac_bundle:
    assert spec['type'] != 'none', (
        'mac_bundle targets cannot have type none (target "%s")' %
        spec['target_name'])
  return is_mac_bundle


def GetMacBundleResources(product_dir, xcode_settings, resources):
  """Yields (output, resource) pairs for every resource in |resources|.
  Only call this for mac bundle targets.

  Args:
      product_dir: Path to the directory containing the output bundle,
          relative to the build directory.
      xcode_settings: The XcodeSettings of the current target.
      resources: A list of bundle resources, relative to the build directory.
  """
  dest = os.path.join(product_dir,
                      xcode_settings.GetBundleResourceFolder())
  for res in resources:
    output = dest

    
    
    assert ' ' not in res, (
      "Spaces in resource filenames not supported (%s)"  % res)

    
    res_parts = os.path.split(res)

    
    lproj_parts = os.path.split(res_parts[0])
    
    if lproj_parts[1].endswith('.lproj'):
      output = os.path.join(output, lproj_parts[1])

    output = os.path.join(output, res_parts[1])
    
    if output.endswith('.xib'):
      output = output[0:-3] + 'nib'

    yield output, res


def GetMacInfoPlist(product_dir, xcode_settings, gyp_path_to_build_path):
  """Returns (info_plist, dest_plist, defines, extra_env), where:
  * |info_plist| is the sourc plist path, relative to the
    build directory,
  * |dest_plist| is the destination plist path, relative to the
    build directory,
  * |defines| is a list of preprocessor defines (empty if the plist
    shouldn't be preprocessed,
  * |extra_env| is a dict of env variables that should be exported when
    invoking |mac_tool copy-info-plist|.

  Only call this for mac bundle targets.

  Args:
      product_dir: Path to the directory containing the output bundle,
          relative to the build directory.
      xcode_settings: The XcodeSettings of the current target.
      gyp_to_build_path: A function that converts paths relative to the
          current gyp file to paths relative to the build direcotry.
  """
  info_plist = xcode_settings.GetPerTargetSetting('INFOPLIST_FILE')
  if not info_plist:
    return None, None, [], {}

  
  
  assert ' ' not in info_plist, (
    "Spaces in Info.plist filenames not supported (%s)"  % info_plist)

  info_plist = gyp_path_to_build_path(info_plist)

  
  
  if xcode_settings.GetPerTargetSetting(
      'INFOPLIST_PREPROCESS', default='NO') == 'YES':
    
    defines = shlex.split(xcode_settings.GetPerTargetSetting(
        'INFOPLIST_PREPROCESSOR_DEFINITIONS', default=''))
  else:
    defines = []

  dest_plist = os.path.join(product_dir, xcode_settings.GetBundlePlistPath())
  extra_env = xcode_settings.GetPerTargetSettings()

  return info_plist, dest_plist, defines, extra_env


def GetXcodeEnv(xcode_settings, built_products_dir, srcroot, configuration,
                additional_settings=None):
  """Return the environment variables that Xcode would set. See
  http://developer.apple.com/library/mac/#documentation/DeveloperTools/Reference/XcodeBuildSettingRef/1-Build_Setting_Reference/build_setting_ref.html#//apple_ref/doc/uid/TP40003931-CH3-SW153
  for a full list.

  Args:
      xcode_settings: An XcodeSettings object. If this is None, this function
          returns an empty dict.
      built_products_dir: Absolute path to the built products dir.
      srcroot: Absolute path to the source root.
      configuration: The build configuration name.
      additional_settings: An optional dict with more values to add to the
          result.
  """
  if not xcode_settings: return {}

  
  
  spec = xcode_settings.spec

  
  env = {
    'BUILT_PRODUCTS_DIR' : built_products_dir,
    'CONFIGURATION' : configuration,
    'PRODUCT_NAME' : xcode_settings.GetProductName(),
    
    'SRCROOT' : srcroot,
    'SOURCE_ROOT': '${SRCROOT}',
    
    
    'TARGET_BUILD_DIR' : built_products_dir,
    'TEMP_DIR' : '${TMPDIR}',
  }
  if spec['type'] in (
      'executable', 'static_library', 'shared_library', 'loadable_module'):
    env['EXECUTABLE_NAME'] = xcode_settings.GetExecutableName()
    env['EXECUTABLE_PATH'] = xcode_settings.GetExecutablePath()
    env['FULL_PRODUCT_NAME'] = xcode_settings.GetFullProductName()
    mach_o_type = xcode_settings.GetMachOType()
    if mach_o_type:
      env['MACH_O_TYPE'] = mach_o_type
    env['PRODUCT_TYPE'] = xcode_settings.GetProductType()
  if xcode_settings._IsBundle():
    env['CONTENTS_FOLDER_PATH'] = \
      xcode_settings.GetBundleContentsFolderPath()
    env['UNLOCALIZED_RESOURCES_FOLDER_PATH'] = \
        xcode_settings.GetBundleResourceFolder()
    env['INFOPLIST_PATH'] = xcode_settings.GetBundlePlistPath()
    env['WRAPPER_NAME'] = xcode_settings.GetWrapperName()

  if not additional_settings:
    additional_settings = {}
  else:
    
    for k in additional_settings:
      if not isinstance(additional_settings[k], str):
        additional_settings[k] = ' '.join(additional_settings[k])
  additional_settings.update(env)

  for k in additional_settings:
    additional_settings[k] = _NormalizeEnvVarReferences(additional_settings[k])

  return additional_settings


def _NormalizeEnvVarReferences(str):
  """Takes a string containing variable references in the form ${FOO}, $(FOO),
  or $FOO, and returns a string with all variable references in the form ${FOO}.
  """
  
  str = re.sub(r'\$([a-zA-Z_][a-zA-Z0-9_]*)', r'${\1}', str)

  
  matches = re.findall(r'(\$\(([a-zA-Z0-9\-_]+)\))', str)
  for match in matches:
    to_replace, variable = match
    assert '$(' not in match, '$($(FOO)) variables not supported: ' + match
    str = str.replace(to_replace, '${' + variable + '}')

  return str


def ExpandEnvVars(string, expansions):
  """Expands ${VARIABLES}, $(VARIABLES), and $VARIABLES in string per the
  expansions dict. If the variable expands to something that references
  another variable, this variable is expanded as well if it's in env --
  until no variables present in env are left."""
  for k in reversed(TopologicallySortedEnvVarKeys(expansions)):
    string = string.replace('${' + k + '}', expansions[k])
    string = string.replace('$(' + k + ')', expansions[k])
    string = string.replace('$' + k, expansions[k])
  return string


def TopologicallySortedEnvVarKeys(env):
  """Takes a dict |env| whose values are strings that can refer to other keys,
  for example env['foo'] = '$(bar) and $(baz)'. Returns a list L of all keys of
  env such that key2 is after key1 in L if env[key2] refers to env[key1].

  Throws an Exception in case of dependency cycles.
  """
  
  
  
  regex = re.compile(r'\$\{([a-zA-Z0-9\-_]+)\}')

  
  key_list = sorted(env.keys())

  
  
  edges = set()
  dependees = set()
  dependers = set()
  for k in key_list:
    matches = regex.findall(env[k])
    if not len(matches):
      continue

    depends_on_other_var = False
    for dependee in matches:
      assert '${' not in dependee, 'Nested variables not supported: ' + dependee
      if dependee in env:
        edges.add((dependee, k))
        dependees.add(dependee)
        depends_on_other_var = True
    if depends_on_other_var:
      dependers.add(k)

  
  sorted_nodes = []
  edgeless_nodes = dependees - dependers

  
  while len(edgeless_nodes):
    
    
    node = edgeless_nodes.pop()
    sorted_nodes.append(node)
    key_list.remove(node)

    
    edges_to_node = [e for e in edges if e[0] == node]
    for edge in edges_to_node:
      edges.remove(edge)
      
      
      if not len([e for e in edges if e[1] == edge[1]]):
        edgeless_nodes.add(edge[1])

  
  if len(edges):
    raise Exception('Xcode environment variables are cyclically dependent: ' +
        str(edges))

  
  sorted_nodes.extend(key_list)

  return sorted_nodes

def GetSpecPostbuildCommands(spec, gyp_path_to_build_path):
  """Returns the list of postbuilds explicitly defined on |spec|, in a form
  executable by a shell."""
  postbuilds = []
  for postbuild in spec.get('postbuilds', []):
    postbuilds.append('echo POSTBUILD\\(%s\\) %s' % (
          spec['target_name'], postbuild['postbuild_name']))
    shell_list = postbuild['action'][:]
    
    
    
    if os.path.sep in shell_list[0]:
      shell_list[0] = gyp_path_to_build_path(shell_list[0])

      
      if not os.path.sep in shell_list[0]:
        shell_list[0] = os.path.join('.', shell_list[0])
    postbuilds.append(gyp.common.EncodePOSIXShellList(shell_list))

  return postbuilds
