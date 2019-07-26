



"""Code to validate and convert settings of the Microsoft build tools.

This file contains code to validate and convert settings of the Microsoft
build tools.  The function ConvertToMSBuildSettings(), ValidateMSVSSettings(),
and ValidateMSBuildSettings() are the entry points.

This file was created by comparing the projects created by Visual Studio 2008
and Visual Studio 2010 for all available settings through the user interface.
The MSBuild schemas were also considered.  They are typically found in the
MSBuild install directory, e.g. c:\Program Files (x86)\MSBuild
"""

import sys
import re



_msvs_validators = {}
_msbuild_validators = {}




_msvs_to_msbuild_converters = {}



_msbuild_name_of_tool = {}


class _Tool(object):
  """Represents a tool used by MSVS or MSBuild.

  Attributes:
      msvs_name: The name of the tool in MSVS.
      msbuild_name: The name of the tool in MSBuild.
  """

  def __init__(self, msvs_name, msbuild_name):
    self.msvs_name = msvs_name
    self.msbuild_name = msbuild_name


def _AddTool(tool):
  """Adds a tool to the four dictionaries used to process settings.

  This only defines the tool.  Each setting also needs to be added.

  Args:
    tool: The _Tool object to be added.
  """
  _msvs_validators[tool.msvs_name] = {}
  _msbuild_validators[tool.msbuild_name] = {}
  _msvs_to_msbuild_converters[tool.msvs_name] = {}
  _msbuild_name_of_tool[tool.msvs_name] = tool.msbuild_name


def _GetMSBuildToolSettings(msbuild_settings, tool):
  """Returns an MSBuild tool dictionary.  Creates it if needed."""
  return msbuild_settings.setdefault(tool.msbuild_name, {})


class _Type(object):
  """Type of settings (Base class)."""

  def ValidateMSVS(self, value):
    """Verifies that the value is legal for MSVS.

    Args:
      value: the value to check for this type.

    Raises:
      ValueError if value is not valid for MSVS.
    """

  def ValidateMSBuild(self, value):
    """Verifies that the value is legal for MSBuild.

    Args:
      value: the value to check for this type.

    Raises:
      ValueError if value is not valid for MSBuild.
    """

  def ConvertToMSBuild(self, value):
    """Returns the MSBuild equivalent of the MSVS value given.

    Args:
      value: the MSVS value to convert.

    Returns:
      the MSBuild equivalent.

    Raises:
      ValueError if value is not valid.
    """
    return value


class _String(_Type):
  """A setting that's just a string."""

  def ValidateMSVS(self, value):
    if not isinstance(value, basestring):
      raise ValueError('expected string; got %r' % value)

  def ValidateMSBuild(self, value):
    if not isinstance(value, basestring):
      raise ValueError('expected string; got %r' % value)

  def ConvertToMSBuild(self, value):
    
    return ConvertVCMacrosToMSBuild(value)


class _StringList(_Type):
  """A settings that's a list of strings."""

  def ValidateMSVS(self, value):
    if not isinstance(value, basestring) and not isinstance(value, list):
      raise ValueError('expected string list; got %r' % value)

  def ValidateMSBuild(self, value):
    if not isinstance(value, basestring) and not isinstance(value, list):
      raise ValueError('expected string list; got %r' % value)

  def ConvertToMSBuild(self, value):
    
    if isinstance(value, list):
      return [ConvertVCMacrosToMSBuild(i) for i in value]
    else:
      return ConvertVCMacrosToMSBuild(value)


class _Boolean(_Type):
  """Boolean settings, can have the values 'false' or 'true'."""

  def _Validate(self, value):
    if value != 'true' and value != 'false':
      raise ValueError('expected bool; got %r' % value)

  def ValidateMSVS(self, value):
    self._Validate(value)

  def ValidateMSBuild(self, value):
    self._Validate(value)

  def ConvertToMSBuild(self, value):
    self._Validate(value)
    return value


class _Integer(_Type):
  """Integer settings."""

  def __init__(self, msbuild_base=10):
    _Type.__init__(self)
    self._msbuild_base = msbuild_base

  def ValidateMSVS(self, value):
    
    self.ConvertToMSBuild(value)

  def ValidateMSBuild(self, value):
    
    int(value, self._msbuild_base)

  def ConvertToMSBuild(self, value):
    msbuild_format = (self._msbuild_base == 10) and '%d' or '0x%04x'
    return msbuild_format % int(value)


class _Enumeration(_Type):
  """Type of settings that is an enumeration.

  In MSVS, the values are indexes like '0', '1', and '2'.
  MSBuild uses text labels that are more representative, like 'Win32'.

  Constructor args:
    label_list: an array of MSBuild labels that correspond to the MSVS index.
        In the rare cases where MSVS has skipped an index value, None is
        used in the array to indicate the unused spot.
    new: an array of labels that are new to MSBuild.
  """

  def __init__(self, label_list, new=None):
    _Type.__init__(self)
    self._label_list = label_list
    self._msbuild_values = set(value for value in label_list
                               if value is not None)
    if new is not None:
      self._msbuild_values.update(new)

  def ValidateMSVS(self, value):
    
    self.ConvertToMSBuild(value)

  def ValidateMSBuild(self, value):
    if value not in self._msbuild_values:
      raise ValueError('unrecognized enumerated value %s' % value)

  def ConvertToMSBuild(self, value):
    index = int(value)
    if index < 0 or index >= len(self._label_list):
      raise ValueError('index value (%d) not in expected range [0, %d)' %
                       (index, len(self._label_list)))
    label = self._label_list[index]
    if label is None:
      raise ValueError('converted value for %s not specified.' % value)
    return label



_boolean = _Boolean()
_integer = _Integer()

_string = _String()
_file_name = _String()
_folder_name = _String()
_file_list = _StringList()
_folder_list = _StringList()
_string_list = _StringList()


_newly_boolean = _Enumeration(['', 'false', 'true'])


def _Same(tool, name, setting_type):
  """Defines a setting that has the same name in MSVS and MSBuild.

  Args:
    tool: a dictionary that gives the names of the tool for MSVS and MSBuild.
    name: the name of the setting.
    setting_type: the type of this setting.
  """
  _Renamed(tool, name, name, setting_type)


def _Renamed(tool, msvs_name, msbuild_name, setting_type):
  """Defines a setting for which the name has changed.

  Args:
    tool: a dictionary that gives the names of the tool for MSVS and MSBuild.
    msvs_name: the name of the MSVS setting.
    msbuild_name: the name of the MSBuild setting.
    setting_type: the type of this setting.
  """

  def _Translate(value, msbuild_settings):
    msbuild_tool_settings = _GetMSBuildToolSettings(msbuild_settings, tool)
    msbuild_tool_settings[msbuild_name] = setting_type.ConvertToMSBuild(value)

  _msvs_validators[tool.msvs_name][msvs_name] = setting_type.ValidateMSVS
  _msbuild_validators[tool.msbuild_name][msbuild_name] = (
      setting_type.ValidateMSBuild)
  _msvs_to_msbuild_converters[tool.msvs_name][msvs_name] = _Translate


def _Moved(tool, settings_name, msbuild_tool_name, setting_type):
  _MovedAndRenamed(tool, settings_name, msbuild_tool_name, settings_name,
                   setting_type)


def _MovedAndRenamed(tool, msvs_settings_name, msbuild_tool_name,
                     msbuild_settings_name, setting_type):
  """Defines a setting that may have moved to a new section.

  Args:
    tool: a dictionary that gives the names of the tool for MSVS and MSBuild.
    msvs_settings_name: the MSVS name of the setting.
    msbuild_tool_name: the name of the MSBuild tool to place the setting under.
    msbuild_settings_name: the MSBuild name of the setting.
    setting_type: the type of this setting.
  """

  def _Translate(value, msbuild_settings):
    tool_settings = msbuild_settings.setdefault(msbuild_tool_name, {})
    tool_settings[msbuild_settings_name] = setting_type.ConvertToMSBuild(value)

  _msvs_validators[tool.msvs_name][msvs_settings_name] = (
      setting_type.ValidateMSVS)
  validator = setting_type.ValidateMSBuild
  _msbuild_validators[msbuild_tool_name][msbuild_settings_name] = validator
  _msvs_to_msbuild_converters[tool.msvs_name][msvs_settings_name] = _Translate


def _MSVSOnly(tool, name, setting_type):
  """Defines a setting that is only found in MSVS.

  Args:
    tool: a dictionary that gives the names of the tool for MSVS and MSBuild.
    name: the name of the setting.
    setting_type: the type of this setting.
  """

  def _Translate(unused_value, unused_msbuild_settings):
    
    pass

  _msvs_validators[tool.msvs_name][name] = setting_type.ValidateMSVS
  _msvs_to_msbuild_converters[tool.msvs_name][name] = _Translate


def _MSBuildOnly(tool, name, setting_type):
  """Defines a setting that is only found in MSBuild.

  Args:
    tool: a dictionary that gives the names of the tool for MSVS and MSBuild.
    name: the name of the setting.
    setting_type: the type of this setting.
  """
  _msbuild_validators[tool.msbuild_name][name] = setting_type.ValidateMSBuild


def _ConvertedToAdditionalOption(tool, msvs_name, flag):
  """Defines a setting that's handled via a command line option in MSBuild.

  Args:
    tool: a dictionary that gives the names of the tool for MSVS and MSBuild.
    msvs_name: the name of the MSVS setting that if 'true' becomes a flag
    flag: the flag to insert at the end of the AdditionalOptions
  """

  def _Translate(value, msbuild_settings):
    if value == 'true':
      tool_settings = _GetMSBuildToolSettings(msbuild_settings, tool)
      if 'AdditionalOptions' in tool_settings:
        new_flags = '%s %s' % (tool_settings['AdditionalOptions'], flag)
      else:
        new_flags = flag
      tool_settings['AdditionalOptions'] = new_flags
  _msvs_validators[tool.msvs_name][msvs_name] = _boolean.ValidateMSVS
  _msvs_to_msbuild_converters[tool.msvs_name][msvs_name] = _Translate


def _CustomGeneratePreprocessedFile(tool, msvs_name):
  def _Translate(value, msbuild_settings):
    tool_settings = _GetMSBuildToolSettings(msbuild_settings, tool)
    if value == '0':
      tool_settings['PreprocessToFile'] = 'false'
      tool_settings['PreprocessSuppressLineNumbers'] = 'false'
    elif value == '1':  
      tool_settings['PreprocessToFile'] = 'true'
      tool_settings['PreprocessSuppressLineNumbers'] = 'false'
    elif value == '2':  
      tool_settings['PreprocessToFile'] = 'true'
      tool_settings['PreprocessSuppressLineNumbers'] = 'true'
    else:
      raise ValueError('value must be one of [0, 1, 2]; got %s' % value)
  
  msvs_validator = _Enumeration(['a', 'b', 'c']).ValidateMSVS
  _msvs_validators[tool.msvs_name][msvs_name] = msvs_validator
  msbuild_validator = _boolean.ValidateMSBuild
  msbuild_tool_validators = _msbuild_validators[tool.msbuild_name]
  msbuild_tool_validators['PreprocessToFile'] = msbuild_validator
  msbuild_tool_validators['PreprocessSuppressLineNumbers'] = msbuild_validator
  _msvs_to_msbuild_converters[tool.msvs_name][msvs_name] = _Translate


fix_vc_macro_slashes_regex_list = ('IntDir', 'OutDir')
fix_vc_macro_slashes_regex = re.compile(
  r'(\$\((?:%s)\))(?:[\\/]+)' % "|".join(fix_vc_macro_slashes_regex_list)
)

def FixVCMacroSlashes(s):
  """Replace macros which have excessive following slashes.

  These macros are known to have a built-in trailing slash. Furthermore, many
  scripts hiccup on processing paths with extra slashes in the middle.

  This list is probably not exhaustive.  Add as needed.
  """
  if '$' in s:
    s = fix_vc_macro_slashes_regex.sub(r'\1', s)
  return s


def ConvertVCMacrosToMSBuild(s):
  """Convert the the MSVS macros found in the string to the MSBuild equivalent.

  This list is probably not exhaustive.  Add as needed.
  """
  if '$' in s:
    replace_map = {
        '$(ConfigurationName)': '$(Configuration)',
        '$(InputDir)': '%(RootDir)%(Directory)',
        '$(InputExt)': '%(Extension)',
        '$(InputFileName)': '%(Filename)%(Extension)',
        '$(InputName)': '%(Filename)',
        '$(InputPath)': '%(FullPath)',
        '$(ParentName)': '$(ProjectFileName)',
        '$(PlatformName)': '$(Platform)',
        '$(SafeInputName)': '%(Filename)',
    }
    for old, new in replace_map.iteritems():
      s = s.replace(old, new)
    s = FixVCMacroSlashes(s)
  return s


def ConvertToMSBuildSettings(msvs_settings, stderr=sys.stderr):
  """Converts MSVS settings (VS2008 and earlier) to MSBuild settings (VS2010+).

  Args:
      msvs_settings: A dictionary.  The key is the tool name.  The values are
          themselves dictionaries of settings and their values.
      stderr: The stream receiving the error messages.

  Returns:
      A dictionary of MSBuild settings.  The key is either the MSBuild tool name
      or the empty string (for the global settings).  The values are themselves
      dictionaries of settings and their values.
  """
  msbuild_settings = {}
  for msvs_tool_name, msvs_tool_settings in msvs_settings.iteritems():
    if msvs_tool_name in _msvs_to_msbuild_converters:
      msvs_tool = _msvs_to_msbuild_converters[msvs_tool_name]
      for msvs_setting, msvs_value in msvs_tool_settings.iteritems():
        if msvs_setting in msvs_tool:
          
          try:
            msvs_tool[msvs_setting](msvs_value, msbuild_settings)
          except ValueError, e:
            print >> stderr, ('Warning: while converting %s/%s to MSBuild, '
                              '%s' % (msvs_tool_name, msvs_setting, e))
        else:
          
          print >> stderr, ('Warning: unrecognized setting %s/%s '
                            'while converting to MSBuild.' %
                            (msvs_tool_name, msvs_setting))
    else:
      print >> stderr, ('Warning: unrecognized tool %s while converting to '
                        'MSBuild.' % msvs_tool_name)
  return msbuild_settings


def ValidateMSVSSettings(settings, stderr=sys.stderr):
  """Validates that the names of the settings are valid for MSVS.

  Args:
      settings: A dictionary.  The key is the tool name.  The values are
          themselves dictionaries of settings and their values.
      stderr: The stream receiving the error messages.
  """
  _ValidateSettings(_msvs_validators, settings, stderr)


def ValidateMSBuildSettings(settings, stderr=sys.stderr):
  """Validates that the names of the settings are valid for MSBuild.

  Args:
      settings: A dictionary.  The key is the tool name.  The values are
          themselves dictionaries of settings and their values.
      stderr: The stream receiving the error messages.
  """
  _ValidateSettings(_msbuild_validators, settings, stderr)


def _ValidateSettings(validators, settings, stderr):
  """Validates that the settings are valid for MSBuild or MSVS.

  We currently only validate the names of the settings, not their values.

  Args:
      validators: A dictionary of tools and their validators.
      settings: A dictionary.  The key is the tool name.  The values are
          themselves dictionaries of settings and their values.
      stderr: The stream receiving the error messages.
  """
  for tool_name in settings:
    if tool_name in validators:
      tool_validators = validators[tool_name]
      for setting, value in settings[tool_name].iteritems():
        if setting in tool_validators:
          try:
            tool_validators[setting](value)
          except ValueError, e:
            print >> stderr, ('Warning: for %s/%s, %s' %
                              (tool_name, setting, e))
        else:
          print >> stderr, ('Warning: unrecognized setting %s/%s' %
                            (tool_name, setting))
    else:
      print >> stderr, ('Warning: unrecognized tool %s' % tool_name)



_compile = _Tool('VCCLCompilerTool', 'ClCompile')
_link = _Tool('VCLinkerTool', 'Link')
_midl = _Tool('VCMIDLTool', 'Midl')
_rc = _Tool('VCResourceCompilerTool', 'ResourceCompile')
_lib = _Tool('VCLibrarianTool', 'Lib')
_manifest = _Tool('VCManifestTool', 'Manifest')


_AddTool(_compile)
_AddTool(_link)
_AddTool(_midl)
_AddTool(_rc)
_AddTool(_lib)
_AddTool(_manifest)

_msbuild_validators[''] = {}
_msbuild_validators['ProjectReference'] = {}
_msbuild_validators['ManifestResourceCompile'] = {}







_Same(_compile, 'AdditionalIncludeDirectories', _folder_list)  
_Same(_compile, 'AdditionalOptions', _string_list)
_Same(_compile, 'AdditionalUsingDirectories', _folder_list)  
_Same(_compile, 'AssemblerListingLocation', _file_name)  
_Same(_compile, 'BrowseInformationFile', _file_name)
_Same(_compile, 'BufferSecurityCheck', _boolean)  
_Same(_compile, 'DisableLanguageExtensions', _boolean)  
_Same(_compile, 'DisableSpecificWarnings', _string_list)  
_Same(_compile, 'EnableFiberSafeOptimizations', _boolean)  
_Same(_compile, 'EnablePREfast', _boolean)  
_Same(_compile, 'ExpandAttributedSource', _boolean)  
_Same(_compile, 'FloatingPointExceptions', _boolean)  
_Same(_compile, 'ForceConformanceInForLoopScope', _boolean)  
_Same(_compile, 'ForcedIncludeFiles', _file_list)  
_Same(_compile, 'ForcedUsingFiles', _file_list)  
_Same(_compile, 'GenerateXMLDocumentationFiles', _boolean)  
_Same(_compile, 'IgnoreStandardIncludePath', _boolean)  
_Same(_compile, 'MinimalRebuild', _boolean)  
_Same(_compile, 'OmitDefaultLibName', _boolean)  
_Same(_compile, 'OmitFramePointers', _boolean)  
_Same(_compile, 'PreprocessorDefinitions', _string_list)  
_Same(_compile, 'ProgramDataBaseFileName', _file_name)  
_Same(_compile, 'RuntimeTypeInfo', _boolean)  
_Same(_compile, 'ShowIncludes', _boolean)  
_Same(_compile, 'SmallerTypeCheck', _boolean)  
_Same(_compile, 'StringPooling', _boolean)  
_Same(_compile, 'SuppressStartupBanner', _boolean)  
_Same(_compile, 'TreatWChar_tAsBuiltInType', _boolean)  
_Same(_compile, 'UndefineAllPreprocessorDefinitions', _boolean)  
_Same(_compile, 'UndefinePreprocessorDefinitions', _string_list)  
_Same(_compile, 'UseFullPaths', _boolean)  
_Same(_compile, 'WholeProgramOptimization', _boolean)  
_Same(_compile, 'XMLDocumentationFileName', _file_name)

_Same(_compile, 'AssemblerOutput',
      _Enumeration(['NoListing',
                    'AssemblyCode',  
                    'All',  
                    'AssemblyAndMachineCode',  
                    'AssemblyAndSourceCode']))  
_Same(_compile, 'BasicRuntimeChecks',
      _Enumeration(['Default',
                    'StackFrameRuntimeCheck',  
                    'UninitializedLocalUsageCheck',  
                    'EnableFastChecks']))  
_Same(_compile, 'BrowseInformation',
      _Enumeration(['false',
                    'true',  
                    'true']))  
_Same(_compile, 'CallingConvention',
      _Enumeration(['Cdecl',  
                    'FastCall',  
                    'StdCall']))  
_Same(_compile, 'CompileAs',
      _Enumeration(['Default',
                    'CompileAsC',  
                    'CompileAsCpp']))  
_Same(_compile, 'DebugInformationFormat',
      _Enumeration(['',  
                    'OldStyle',  
                    None,
                    'ProgramDatabase',  
                    'EditAndContinue']))  
_Same(_compile, 'EnableEnhancedInstructionSet',
      _Enumeration(['NotSet',
                    'StreamingSIMDExtensions',  
                    'StreamingSIMDExtensions2']))  
_Same(_compile, 'ErrorReporting',
      _Enumeration(['None',  
                    'Prompt',  
                    'Queue'],  
                   new=['Send']))  
_Same(_compile, 'ExceptionHandling',
      _Enumeration(['false',
                    'Sync',  
                    'Async'],  
                   new=['SyncCThrow']))  
_Same(_compile, 'FavorSizeOrSpeed',
      _Enumeration(['Neither',
                    'Speed',  
                    'Size']))  
_Same(_compile, 'FloatingPointModel',
      _Enumeration(['Precise',  
                    'Strict',  
                    'Fast']))  
_Same(_compile, 'InlineFunctionExpansion',
      _Enumeration(['Default',
                    'OnlyExplicitInline',  
                    'AnySuitable'],  
                   new=['Disabled']))  
_Same(_compile, 'Optimization',
      _Enumeration(['Disabled',  
                    'MinSpace',  
                    'MaxSpeed',  
                    'Full']))  
_Same(_compile, 'RuntimeLibrary',
      _Enumeration(['MultiThreaded',  
                    'MultiThreadedDebug',  
                    'MultiThreadedDLL',  
                    'MultiThreadedDebugDLL']))  
_Same(_compile, 'StructMemberAlignment',
      _Enumeration(['Default',
                    '1Byte',  
                    '2Bytes',  
                    '4Bytes',  
                    '8Bytes',  
                    '16Bytes']))  
_Same(_compile, 'WarningLevel',
      _Enumeration(['TurnOffAllWarnings',  
                    'Level1',  
                    'Level2',  
                    'Level3',  
                    'Level4'],  
                   new=['EnableAllWarnings']))  


_Renamed(_compile, 'EnableFunctionLevelLinking', 'FunctionLevelLinking',
         _boolean)  
_Renamed(_compile, 'EnableIntrinsicFunctions', 'IntrinsicFunctions',
         _boolean)  
_Renamed(_compile, 'KeepComments', 'PreprocessKeepComments', _boolean)  
_Renamed(_compile, 'ObjectFile', 'ObjectFileName', _file_name)  
_Renamed(_compile, 'OpenMP', 'OpenMPSupport', _boolean)  
_Renamed(_compile, 'PrecompiledHeaderThrough', 'PrecompiledHeaderFile',
         _file_name)  
_Renamed(_compile, 'PrecompiledHeaderFile', 'PrecompiledHeaderOutputFile',
         _file_name)  
_Renamed(_compile, 'UsePrecompiledHeader', 'PrecompiledHeader',
         _Enumeration(['NotUsing',  
                       'Create',   
                       'Use']))  
_Renamed(_compile, 'WarnAsError', 'TreatWarningAsError', _boolean)  

_ConvertedToAdditionalOption(_compile, 'DefaultCharIsUnsigned', '/J')


_MSVSOnly(_compile, 'Detect64BitPortabilityProblems', _boolean)
_MSVSOnly(_compile, 'UseUnicodeResponseFiles', _boolean)


_MSBuildOnly(_compile, 'BuildingInIDE', _boolean)
_MSBuildOnly(_compile, 'CompileAsManaged',
             _Enumeration([], new=['false',
                                   'true',  
                                   'Pure',  
                                   'Safe',  
                                   'OldSyntax']))  
_MSBuildOnly(_compile, 'CreateHotpatchableImage', _boolean)  
_MSBuildOnly(_compile, 'MultiProcessorCompilation', _boolean)  
_MSBuildOnly(_compile, 'PreprocessOutputPath', _string)  
_MSBuildOnly(_compile, 'ProcessorNumber', _integer)  
_MSBuildOnly(_compile, 'TrackerLogDirectory', _folder_name)
_MSBuildOnly(_compile, 'TreatSpecificWarningsAsErrors', _string_list)  
_MSBuildOnly(_compile, 'UseUnicodeForAssemblerListing', _boolean)  


_CustomGeneratePreprocessedFile(_compile, 'GeneratePreprocessedFile')







_Same(_link, 'AdditionalDependencies', _file_list)
_Same(_link, 'AdditionalLibraryDirectories', _folder_list)  

_Same(_link, 'AdditionalManifestDependencies', _file_list)
_Same(_link, 'AdditionalOptions', _string_list)
_Same(_link, 'AddModuleNamesToAssembly', _file_list)  
_Same(_link, 'AllowIsolation', _boolean)  
_Same(_link, 'AssemblyLinkResource', _file_list)  
_Same(_link, 'BaseAddress', _string)  
_Same(_link, 'CLRUnmanagedCodeCheck', _boolean)  
_Same(_link, 'DelayLoadDLLs', _file_list)  
_Same(_link, 'DelaySign', _boolean)  
_Same(_link, 'EmbedManagedResourceFile', _file_list)  
_Same(_link, 'EnableUAC', _boolean)  
_Same(_link, 'EntryPointSymbol', _string)  
_Same(_link, 'ForceSymbolReferences', _file_list)  
_Same(_link, 'FunctionOrder', _file_name)  
_Same(_link, 'GenerateDebugInformation', _boolean)  
_Same(_link, 'GenerateMapFile', _boolean)  
_Same(_link, 'HeapCommitSize', _string)
_Same(_link, 'HeapReserveSize', _string)  
_Same(_link, 'IgnoreAllDefaultLibraries', _boolean)  
_Same(_link, 'IgnoreEmbeddedIDL', _boolean)  
_Same(_link, 'ImportLibrary', _file_name)  
_Same(_link, 'KeyContainer', _file_name)  
_Same(_link, 'KeyFile', _file_name)  
_Same(_link, 'ManifestFile', _file_name)  
_Same(_link, 'MapExports', _boolean)  
_Same(_link, 'MapFileName', _file_name)
_Same(_link, 'MergedIDLBaseFileName', _file_name)  
_Same(_link, 'MergeSections', _string)  
_Same(_link, 'MidlCommandFile', _file_name)  
_Same(_link, 'ModuleDefinitionFile', _file_name)  
_Same(_link, 'OutputFile', _file_name)  
_Same(_link, 'PerUserRedirection', _boolean)
_Same(_link, 'Profile', _boolean)  
_Same(_link, 'ProfileGuidedDatabase', _file_name)  
_Same(_link, 'ProgramDatabaseFile', _file_name)  
_Same(_link, 'RegisterOutput', _boolean)
_Same(_link, 'SetChecksum', _boolean)  
_Same(_link, 'StackCommitSize', _string)
_Same(_link, 'StackReserveSize', _string)  
_Same(_link, 'StripPrivateSymbols', _file_name)  
_Same(_link, 'SupportUnloadOfDelayLoadedDLL', _boolean)  
_Same(_link, 'SuppressStartupBanner', _boolean)  
_Same(_link, 'SwapRunFromCD', _boolean)  
_Same(_link, 'TurnOffAssemblyGeneration', _boolean)  
_Same(_link, 'TypeLibraryFile', _file_name)  
_Same(_link, 'TypeLibraryResourceID', _integer)  
_Same(_link, 'UACUIAccess', _boolean)  
_Same(_link, 'Version', _string)  

_Same(_link, 'EnableCOMDATFolding', _newly_boolean)  
_Same(_link, 'FixedBaseAddress', _newly_boolean)  
_Same(_link, 'LargeAddressAware', _newly_boolean)  
_Same(_link, 'OptimizeReferences', _newly_boolean)  
_Same(_link, 'RandomizedBaseAddress', _newly_boolean)  
_Same(_link, 'TerminalServerAware', _newly_boolean)  

_subsystem_enumeration = _Enumeration(
    ['NotSet',
     'Console',  
     'Windows',  
     'Native',  
     'EFI Application',  
     'EFI Boot Service Driver',  
     'EFI ROM',  
     'EFI Runtime',  
     'WindowsCE'],  
    new=['POSIX'])  

_target_machine_enumeration = _Enumeration(
    ['NotSet',
     'MachineX86',  
     None,
     'MachineARM',  
     'MachineEBC',  
     'MachineIA64',  
     None,
     'MachineMIPS',  
     'MachineMIPS16',  
     'MachineMIPSFPU',  
     'MachineMIPSFPU16',  
     None,
     None,
     None,
     'MachineSH4',  
     None,
     'MachineTHUMB',  
     'MachineX64'])  

_Same(_link, 'AssemblyDebug',
      _Enumeration(['',
                    'true',  
                    'false']))  
_Same(_link, 'CLRImageType',
      _Enumeration(['Default',
                    'ForceIJWImage',  
                    'ForcePureILImage',  
                    'ForceSafeILImage']))  
_Same(_link, 'CLRThreadAttribute',
      _Enumeration(['DefaultThreadingAttribute',  
                    'MTAThreadingAttribute',  
                    'STAThreadingAttribute']))  
_Same(_link, 'DataExecutionPrevention',
      _Enumeration(['',
                    'false',  
                    'true']))  
_Same(_link, 'Driver',
      _Enumeration(['NotSet',
                    'Driver',  
                    'UpOnly',  
                    'WDM']))  
_Same(_link, 'LinkTimeCodeGeneration',
      _Enumeration(['Default',
                    'UseLinkTimeCodeGeneration',  
                    'PGInstrument',  
                    'PGOptimization',  
                    'PGUpdate']))  
_Same(_link, 'ShowProgress',
      _Enumeration(['NotSet',
                    'LinkVerbose',  
                    'LinkVerboseLib'],  
                   new=['LinkVerboseICF',  
                        'LinkVerboseREF',  
                        'LinkVerboseSAFESEH',  
                        'LinkVerboseCLR']))  
_Same(_link, 'SubSystem', _subsystem_enumeration)
_Same(_link, 'TargetMachine', _target_machine_enumeration)
_Same(_link, 'UACExecutionLevel',
      _Enumeration(['AsInvoker',  
                    'HighestAvailable',  
                    'RequireAdministrator']))  



_Renamed(_link, 'ErrorReporting', 'LinkErrorReporting',
         _Enumeration(['NoErrorReport',  
                       'PromptImmediately',  
                       'QueueForNextLogin'],  
                      new=['SendErrorReport']))  
_Renamed(_link, 'IgnoreDefaultLibraryNames', 'IgnoreSpecificDefaultLibraries',
         _file_list)  
_Renamed(_link, 'ResourceOnlyDLL', 'NoEntryPoint', _boolean)  
_Renamed(_link, 'SwapRunFromNet', 'SwapRunFromNET', _boolean)  

_Moved(_link, 'GenerateManifest', '', _boolean)
_Moved(_link, 'IgnoreImportLibrary', '', _boolean)
_Moved(_link, 'LinkIncremental', '', _newly_boolean)
_Moved(_link, 'LinkLibraryDependencies', 'ProjectReference', _boolean)
_Moved(_link, 'UseLibraryDependencyInputs', 'ProjectReference', _boolean)


_MSVSOnly(_link, 'OptimizeForWindows98', _newly_boolean)
_MSVSOnly(_link, 'UseUnicodeResponseFiles', _boolean)

_MSVSOnly(_link, 'AdditionalLibraryDirectories_excluded', _folder_list)


_MSBuildOnly(_link, 'BuildingInIDE', _boolean)
_MSBuildOnly(_link, 'ImageHasSafeExceptionHandlers', _boolean)  
_MSBuildOnly(_link, 'LinkDLL', _boolean)  
_MSBuildOnly(_link, 'LinkStatus', _boolean)  
_MSBuildOnly(_link, 'PreventDllBinding', _boolean)  
_MSBuildOnly(_link, 'SupportNobindOfDelayLoadedDLL', _boolean)  
_MSBuildOnly(_link, 'TrackerLogDirectory', _folder_name)
_MSBuildOnly(_link, 'TreatLinkerWarningAsErrors', _boolean)  
_MSBuildOnly(_link, 'MinimumRequiredVersion', _string)
_MSBuildOnly(_link, 'MSDOSStubFileName', _file_name)  
_MSBuildOnly(_link, 'SectionAlignment', _integer)  
_MSBuildOnly(_link, 'SpecifySectionAttributes', _string)  
_MSBuildOnly(_link, 'ForceFileOutput',
             _Enumeration([], new=['Enabled',  
                                   
                                   'MultiplyDefinedSymbolOnly',
                                   'UndefinedSymbolOnly']))  
_MSBuildOnly(_link, 'CreateHotPatchableImage',
             _Enumeration([], new=['Enabled',  
                                   'X86Image',  
                                   'X64Image',  
                                   'ItaniumImage']))  
_MSBuildOnly(_link, 'CLRSupportLastError',
             _Enumeration([], new=['Enabled',  
                                   'Disabled',  
                                   
                                   'SystemDlls']))






_Same(_rc, 'AdditionalOptions', _string_list)
_Same(_rc, 'AdditionalIncludeDirectories', _folder_list)  
_Same(_rc, 'Culture', _Integer(msbuild_base=16))
_Same(_rc, 'IgnoreStandardIncludePath', _boolean)  
_Same(_rc, 'PreprocessorDefinitions', _string_list)  
_Same(_rc, 'ResourceOutputFileName', _string)  
_Same(_rc, 'ShowProgress', _boolean)  



_Same(_rc, 'SuppressStartupBanner', _boolean)  
_Same(_rc, 'UndefinePreprocessorDefinitions', _string_list)  


_MSBuildOnly(_rc, 'NullTerminateStrings', _boolean)  
_MSBuildOnly(_rc, 'TrackerLogDirectory', _folder_name)






_Same(_midl, 'AdditionalIncludeDirectories', _folder_list)  
_Same(_midl, 'AdditionalOptions', _string_list)
_Same(_midl, 'CPreprocessOptions', _string)  
_Same(_midl, 'ErrorCheckAllocations', _boolean)  
_Same(_midl, 'ErrorCheckBounds', _boolean)  
_Same(_midl, 'ErrorCheckEnumRange', _boolean)  
_Same(_midl, 'ErrorCheckRefPointers', _boolean)  
_Same(_midl, 'ErrorCheckStubData', _boolean)  
_Same(_midl, 'GenerateStublessProxies', _boolean)  
_Same(_midl, 'GenerateTypeLibrary', _boolean)
_Same(_midl, 'HeaderFileName', _file_name)  
_Same(_midl, 'IgnoreStandardIncludePath', _boolean)  
_Same(_midl, 'InterfaceIdentifierFileName', _file_name)  
_Same(_midl, 'MkTypLibCompatible', _boolean)  
_Same(_midl, 'OutputDirectory', _string)  
_Same(_midl, 'PreprocessorDefinitions', _string_list)  
_Same(_midl, 'ProxyFileName', _file_name)  
_Same(_midl, 'RedirectOutputAndErrors', _file_name)  
_Same(_midl, 'SuppressStartupBanner', _boolean)  
_Same(_midl, 'TypeLibraryName', _file_name)  
_Same(_midl, 'UndefinePreprocessorDefinitions', _string_list)  
_Same(_midl, 'WarnAsError', _boolean)  

_Same(_midl, 'DefaultCharType',
      _Enumeration(['Unsigned',  
                    'Signed',  
                    'Ascii']))  
_Same(_midl, 'TargetEnvironment',
      _Enumeration(['NotSet',
                    'Win32',  
                    'Itanium',  
                    'X64']))  
_Same(_midl, 'EnableErrorChecks',
      _Enumeration(['EnableCustom',
                    'None',  
                    'All']))  
_Same(_midl, 'StructMemberAlignment',
      _Enumeration(['NotSet',
                    '1',  
                    '2',  
                    '4',  
                    '8']))  
_Same(_midl, 'WarningLevel',
      _Enumeration(['0',  
                    '1',  
                    '2',  
                    '3',  
                    '4']))  

_Renamed(_midl, 'DLLDataFileName', 'DllDataFileName', _file_name)  
_Renamed(_midl, 'ValidateParameters', 'ValidateAllParameters',
         _boolean)  


_MSBuildOnly(_midl, 'ApplicationConfigurationMode', _boolean)  
_MSBuildOnly(_midl, 'ClientStubFile', _file_name)  
_MSBuildOnly(_midl, 'GenerateClientFiles',
             _Enumeration([], new=['Stub',  
                                   'None']))  
_MSBuildOnly(_midl, 'GenerateServerFiles',
             _Enumeration([], new=['Stub',  
                                   'None']))  
_MSBuildOnly(_midl, 'LocaleID', _integer)  
_MSBuildOnly(_midl, 'ServerStubFile', _file_name)  
_MSBuildOnly(_midl, 'SuppressCompilerWarnings', _boolean)  
_MSBuildOnly(_midl, 'TrackerLogDirectory', _folder_name)
_MSBuildOnly(_midl, 'TypeLibFormat',
             _Enumeration([], new=['NewFormat',  
                                   'OldFormat']))  






_Same(_lib, 'AdditionalDependencies', _file_list)
_Same(_lib, 'AdditionalLibraryDirectories', _folder_list)  
_Same(_lib, 'AdditionalOptions', _string_list)
_Same(_lib, 'ExportNamedFunctions', _string_list)  
_Same(_lib, 'ForceSymbolReferences', _string)  
_Same(_lib, 'IgnoreAllDefaultLibraries', _boolean)  
_Same(_lib, 'IgnoreSpecificDefaultLibraries', _file_list)  
_Same(_lib, 'ModuleDefinitionFile', _file_name)  
_Same(_lib, 'OutputFile', _file_name)  
_Same(_lib, 'SuppressStartupBanner', _boolean)  
_Same(_lib, 'UseUnicodeResponseFiles', _boolean)
_Same(_lib, 'LinkTimeCodeGeneration', _boolean)  



_Moved(_lib, 'LinkLibraryDependencies', 'ProjectReference', _boolean)


_MSVSOnly(_lib, 'AdditionalLibraryDirectories_excluded', _folder_list)

_MSBuildOnly(_lib, 'DisplayLibrary', _string)  
_MSBuildOnly(_lib, 'ErrorReporting',
             _Enumeration([], new=['PromptImmediately',  
                                   'QueueForNextLogin',  
                                   'SendErrorReport',  
                                   'NoErrorReport']))  
_MSBuildOnly(_lib, 'MinimumRequiredVersion', _string)
_MSBuildOnly(_lib, 'Name', _file_name)  
_MSBuildOnly(_lib, 'RemoveObjects', _file_list)  
_MSBuildOnly(_lib, 'SubSystem', _subsystem_enumeration)
_MSBuildOnly(_lib, 'TargetMachine', _target_machine_enumeration)
_MSBuildOnly(_lib, 'TrackerLogDirectory', _folder_name)
_MSBuildOnly(_lib, 'TreatLibWarningAsErrors', _boolean)  
_MSBuildOnly(_lib, 'Verbose', _boolean)







_Same(_manifest, 'AdditionalManifestFiles', _file_list)  
_Same(_manifest, 'AdditionalOptions', _string_list)
_Same(_manifest, 'AssemblyIdentity', _string)  
_Same(_manifest, 'ComponentFileName', _file_name)  
_Same(_manifest, 'GenerateCatalogFiles', _boolean)  
_Same(_manifest, 'InputResourceManifests', _string)  
_Same(_manifest, 'OutputManifestFile', _file_name)  
_Same(_manifest, 'RegistrarScriptFile', _file_name)  
_Same(_manifest, 'ReplacementsFile', _file_name)  
_Same(_manifest, 'SuppressStartupBanner', _boolean)  
_Same(_manifest, 'TypeLibraryFile', _file_name)  
_Same(_manifest, 'UpdateFileHashes', _boolean)  
_Same(_manifest, 'UpdateFileHashesSearchPath', _file_name)
_Same(_manifest, 'VerboseOutput', _boolean)  


_MovedAndRenamed(_manifest, 'ManifestResourceFile',
                 'ManifestResourceCompile',
                 'ResourceOutputFileName',
                 _file_name)
_Moved(_manifest, 'EmbedManifest', '', _boolean)


_MSVSOnly(_manifest, 'DependencyInformationFile', _file_name)
_MSVSOnly(_manifest, 'UseFAT32Workaround', _boolean)
_MSVSOnly(_manifest, 'UseUnicodeResponseFiles', _boolean)


_MSBuildOnly(_manifest, 'EnableDPIAwareness', _boolean)
_MSBuildOnly(_manifest, 'GenerateCategoryTags', _boolean)  
_MSBuildOnly(_manifest, 'ManifestFromManagedAssembly',
             _file_name)  
_MSBuildOnly(_manifest, 'OutputResourceManifests', _string)  
_MSBuildOnly(_manifest, 'SuppressDependencyElement', _boolean)  
_MSBuildOnly(_manifest, 'TrackerLogDirectory', _folder_name)
