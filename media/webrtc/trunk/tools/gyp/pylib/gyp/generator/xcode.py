



import filecmp
import gyp.common
import gyp.xcodeproj_file
import errno
import os
import sys
import posixpath
import re
import shutil
import subprocess
import tempfile











_intermediate_var = 'INTERMEDIATE_DIR'



_shared_intermediate_var = 'SHARED_INTERMEDIATE_DIR'

_library_search_paths_var = 'LIBRARY_SEARCH_PATHS'

generator_default_variables = {
  'EXECUTABLE_PREFIX': '',
  'EXECUTABLE_SUFFIX': '',
  'STATIC_LIB_PREFIX': 'lib',
  'SHARED_LIB_PREFIX': 'lib',
  'STATIC_LIB_SUFFIX': '.a',
  'SHARED_LIB_SUFFIX': '.dylib',
  
  
  
  
  
  'INTERMEDIATE_DIR': '$(%s)' % _intermediate_var,
  'OS': 'mac',
  'PRODUCT_DIR': '$(BUILT_PRODUCTS_DIR)',
  'LIB_DIR': '$(BUILT_PRODUCTS_DIR)',
  'RULE_INPUT_ROOT': '$(INPUT_FILE_BASE)',
  'RULE_INPUT_EXT': '$(INPUT_FILE_SUFFIX)',
  'RULE_INPUT_NAME': '$(INPUT_FILE_NAME)',
  'RULE_INPUT_PATH': '$(INPUT_FILE_PATH)',
  'RULE_INPUT_DIRNAME': '$(INPUT_FILE_DIRNAME)',
  'SHARED_INTERMEDIATE_DIR': '$(%s)' % _shared_intermediate_var,
  'CONFIGURATION_NAME': '$(CONFIGURATION)',
}


generator_additional_path_sections = [
  'mac_bundle_resources',
  'mac_framework_headers',
  'mac_framework_private_headers',
  
]



generator_additional_non_configuration_keys = [
  'mac_bundle',
  'mac_bundle_resources',
  'mac_framework_headers',
  'mac_framework_private_headers',
  'xcode_create_dependents_test_runner',
]


generator_extra_sources_for_rules = [
  'mac_bundle_resources',
  'mac_framework_headers',
  'mac_framework_private_headers',
]



xcode_standard_library_dirs = frozenset([
  '$(SDKROOT)/usr/lib',
  '$(SDKROOT)/usr/local/lib',
])

def CreateXCConfigurationList(configuration_names):
  xccl = gyp.xcodeproj_file.XCConfigurationList({'buildConfigurations': []})
  if len(configuration_names) == 0:
    configuration_names = ['Default']
  for configuration_name in configuration_names:
    xcbc = gyp.xcodeproj_file.XCBuildConfiguration({
        'name': configuration_name})
    xccl.AppendProperty('buildConfigurations', xcbc)
  xccl.SetProperty('defaultConfigurationName', configuration_names[0])
  return xccl


class XcodeProject(object):
  def __init__(self, gyp_path, path, build_file_dict):
    self.gyp_path = gyp_path
    self.path = path
    self.project = gyp.xcodeproj_file.PBXProject(path=path)
    projectDirPath = gyp.common.RelativePath(
                         os.path.dirname(os.path.abspath(self.gyp_path)),
                         os.path.dirname(path) or '.')
    self.project.SetProperty('projectDirPath', projectDirPath)
    self.project_file = \
        gyp.xcodeproj_file.XCProjectFile({'rootObject': self.project})
    self.build_file_dict = build_file_dict

    
    
    
    self.created_dir = False
    try:
      os.makedirs(self.path)
      self.created_dir = True
    except OSError, e:
      if e.errno != errno.EEXIST:
        raise

  def Finalize1(self, xcode_targets, serialize_all_tests):
    
    
    
    
    configurations = []
    for xct in self.project.GetProperty('targets'):
      xccl = xct.GetProperty('buildConfigurationList')
      xcbcs = xccl.GetProperty('buildConfigurations')
      for xcbc in xcbcs:
        name = xcbc.GetProperty('name')
        if name not in configurations:
          configurations.append(name)

    
    
    
    try:
      xccl = CreateXCConfigurationList(configurations)
      self.project.SetProperty('buildConfigurationList', xccl)
    except:
      sys.stderr.write("Problem with gyp file %s\n" % self.gyp_path)
      raise

    
    
    
    
    
    
    
    
    
    
    
    
    xccl.SetBuildSetting(_intermediate_var,
                         '$(PROJECT_DERIVED_FILE_DIR)/$(CONFIGURATION)')
    xccl.SetBuildSetting(_shared_intermediate_var,
                         '$(SYMROOT)/DerivedSources/$(CONFIGURATION)')

    
    
    
    
    
    
    
    
    
    for xck, xcv in self.build_file_dict.get('xcode_settings', {}).iteritems():
      xccl.SetBuildSetting(xck, xcv)
    if 'xcode_config_file' in self.build_file_dict:
      config_ref = self.project.AddOrGetFileInRootGroup(
          self.build_file_dict['xcode_config_file'])
      xccl.SetBaseConfiguration(config_ref)
    build_file_configurations = self.build_file_dict.get('configurations', {})
    if build_file_configurations:
      for config_name in configurations:
        build_file_configuration_named = \
            build_file_configurations.get(config_name, {})
        if build_file_configuration_named:
          xcc = xccl.ConfigurationNamed(config_name)
          for xck, xcv in build_file_configuration_named.get('xcode_settings',
                                                             {}).iteritems():
            xcc.SetBuildSetting(xck, xcv)
          if 'xcode_config_file' in build_file_configuration_named:
            config_ref = self.project.AddOrGetFileInRootGroup(
                build_file_configurations[config_name]['xcode_config_file'])
            xcc.SetBaseConfiguration(config_ref)

    
    
    

    
    
    
    
    ordinary_targets = []
    run_test_targets = []
    support_targets = []

    
    targets = []

    
    has_custom_all = False

    
    
    
    targets_for_all = []

    for target in self.build_file_dict['targets']:
      target_name = target['target_name']
      toolset = target['toolset']
      qualified_target = gyp.common.QualifiedTarget(self.gyp_path, target_name,
                                                    toolset)
      xcode_target = xcode_targets[qualified_target]
      
      
      assert xcode_target in self.project._properties['targets']
      targets.append(xcode_target)
      ordinary_targets.append(xcode_target)
      if xcode_target.support_target:
        support_targets.append(xcode_target.support_target)
        targets.append(xcode_target.support_target)

      if not int(target.get('suppress_wildcard', False)):
        targets_for_all.append(xcode_target)

      if target_name.lower() == 'all':
        has_custom_all = True;

      
      
      if target.get('run_as'):
        
        
        xccl = CreateXCConfigurationList(configurations)
        run_target = gyp.xcodeproj_file.PBXAggregateTarget({
              'name':                   'Run ' + target_name,
              'productName':            xcode_target.GetProperty('productName'),
              'buildConfigurationList': xccl,
            },
            parent=self.project)
        run_target.AddDependency(xcode_target)

        command = target['run_as']
        script = ''
        if command.get('working_directory'):
          script = script + 'cd "%s"\n' % \
                   gyp.xcodeproj_file.ConvertVariablesToShellSyntax(
                       command.get('working_directory'))

        if command.get('environment'):
          script = script + "\n".join(
            ['export %s="%s"' %
             (key, gyp.xcodeproj_file.ConvertVariablesToShellSyntax(val))
             for (key, val) in command.get('environment').iteritems()]) + "\n"

        
        
        
        
        
        
        command_prefix = ''
        if serialize_all_tests:
          command_prefix = \
"""python -c "import fcntl, subprocess, sys
file = open('$TMPDIR/GYP_serialize_test_runs', 'a')
fcntl.flock(file.fileno(), fcntl.LOCK_EX)
sys.exit(subprocess.call(sys.argv[1:]))" """

        
        
        
        script = script + 'exec ' + command_prefix + '%s\nexit 1\n' % \
                 gyp.xcodeproj_file.ConvertVariablesToShellSyntax(
                     gyp.common.EncodePOSIXShellList(command.get('action')))

        ssbp = gyp.xcodeproj_file.PBXShellScriptBuildPhase({
              'shellScript':      script,
              'showEnvVarsInLog': 0,
            })
        run_target.AppendProperty('buildPhases', ssbp)

        
        targets.append(run_target)
        run_test_targets.append(run_target)
        xcode_target.test_runner = run_target


    
    
    assert len(self.project._properties['targets']) == \
      len(ordinary_targets) + len(support_targets)

    self.project._properties['targets'] = targets

    
    self.project.RootGroupsTakeOverOnlyChildren(True)

    
    
    self.project.SortGroups()

    
    
    
    
    if len(targets_for_all) > 1 and not has_custom_all:
      xccl = CreateXCConfigurationList(configurations)
      all_target = gyp.xcodeproj_file.PBXAggregateTarget(
          {
            'buildConfigurationList': xccl,
            'name':                   'All',
          },
          parent=self.project)

      for target in targets_for_all:
        all_target.AddDependency(target)

      
      
      
      self.project._properties['targets'].insert(0, all_target)

    
    if len(run_test_targets) > 1:
      xccl = CreateXCConfigurationList(configurations)
      run_all_tests_target = gyp.xcodeproj_file.PBXAggregateTarget(
          {
            'buildConfigurationList': xccl,
            'name':                   'Run All Tests',
          },
          parent=self.project)
      for run_test_target in run_test_targets:
        run_all_tests_target.AddDependency(run_test_target)

      
      
      self.project._properties['targets'].insert(1, run_all_tests_target)

  def Finalize2(self, xcode_targets, xcode_target_to_target_dict):
    
    
    
    
    

    
    
    
    
    
    for bf_tgt in self.build_file_dict['targets']:
      if int(bf_tgt.get('xcode_create_dependents_test_runner', 0)):
        tgt_name = bf_tgt['target_name']
        toolset = bf_tgt['toolset']
        qualified_target = gyp.common.QualifiedTarget(self.gyp_path,
                                                      tgt_name, toolset)
        xcode_target = xcode_targets[qualified_target]
        if isinstance(xcode_target, gyp.xcodeproj_file.PBXAggregateTarget):
          
          all_run_tests = []
          pbxtds = xcode_target.GetProperty('dependencies')
          for pbxtd in pbxtds:
            pbxcip = pbxtd.GetProperty('targetProxy')
            dependency_xct = pbxcip.GetProperty('remoteGlobalIDString')
            if hasattr(dependency_xct, 'test_runner'):
              all_run_tests.append(dependency_xct.test_runner)

          
          
          if len(all_run_tests) > 0:
            run_all_target = gyp.xcodeproj_file.PBXAggregateTarget({
                  'name':        'Run %s Tests' % tgt_name,
                  'productName': tgt_name,
                },
                parent=self.project)
            for run_test_target in all_run_tests:
              run_all_target.AddDependency(run_test_target)

            
            idx = self.project._properties['targets'].index(xcode_target)
            self.project._properties['targets'].insert(idx + 1, run_all_target)

    
    
    
    
    
    for other_pbxproject in self.project._other_pbxprojects.keys():
      self.project.AddOrGetProjectReference(other_pbxproject)

    self.project.SortRemoteProductReferences()

    
    self.project_file.ComputeIDs()

    
    
    
    
    self.project_file.EnsureNoIDCollisions()

  def Write(self):
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    (output_fd, new_pbxproj_path) = \
        tempfile.mkstemp(suffix='.tmp', prefix='project.pbxproj.gyp.',
                         dir=self.path)

    try:
      output_file = os.fdopen(output_fd, 'wb')

      self.project_file.Print(output_file)
      output_file.close()

      pbxproj_path = os.path.join(self.path, 'project.pbxproj')

      same = False
      try:
        same = filecmp.cmp(pbxproj_path, new_pbxproj_path, False)
      except OSError, e:
        if e.errno != errno.ENOENT:
          raise

      if same:
        
        
        os.unlink(new_pbxproj_path)
      else:
        
        
        
        
        
        
        
        
        
        
        umask = os.umask(077)
        os.umask(umask)

        os.chmod(new_pbxproj_path, 0666 & ~umask)
        os.rename(new_pbxproj_path, pbxproj_path)

    except Exception:
      
      
      os.unlink(new_pbxproj_path)
      if self.created_dir:
        shutil.rmtree(self.path, True)
      raise


cached_xcode_version = None
def InstalledXcodeVersion():
  """Fetches the installed version of Xcode, returns empty string if it is
  unable to figure it out."""

  global cached_xcode_version
  if not cached_xcode_version is None:
    return cached_xcode_version

  
  cached_xcode_version = ''

  
  try:
    import subprocess
    cmd = ['/usr/bin/xcodebuild', '-version']
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    xcodebuild_version_info = proc.communicate()[0]
    
    if proc.returncode:
      xcodebuild_version_info = ''
  except OSError:
    
    xcodebuild_version_info = ''

  
  match_line = re.search('^Xcode (.*)$', xcodebuild_version_info, re.MULTILINE)
  if match_line:
    cached_xcode_version = match_line.group(1)
  
  return cached_xcode_version


def AddSourceToTarget(source, type, pbxp, xct):
  
  
  source_extensions = ['c', 'cc', 'cpp', 'cxx', 'm', 'mm', 's']

  
  
  
  
  library_extensions = ['a', 'dylib', 'framework', 'o']

  basename = posixpath.basename(source)
  (root, ext) = posixpath.splitext(basename)
  if ext:
    ext = ext[1:].lower()

  if ext in source_extensions and type != 'none':
    xct.SourcesPhase().AddFile(source)
  elif ext in library_extensions and type != 'none':
    xct.FrameworksPhase().AddFile(source)
  else:
    
    
    pbxp.AddOrGetFileInRootGroup(source)


def AddResourceToTarget(resource, pbxp, xct):
  
  
  xct.ResourcesPhase().AddFile(resource)


def AddHeaderToTarget(header, pbxp, xct, is_public):
  
  
  settings = '{ATTRIBUTES = (%s, ); }' % ('Private', 'Public')[is_public]
  xct.HeadersPhase().AddFile(header, settings)


_xcode_variable_re = re.compile('(\$\((.*?)\))')
def ExpandXcodeVariables(string, expansions):
  """Expands Xcode-style $(VARIABLES) in string per the expansions dict.

  In some rare cases, it is appropriate to expand Xcode variables when a
  project file is generated.  For any substring $(VAR) in string, if VAR is a
  key in the expansions dict, $(VAR) will be replaced with expansions[VAR].
  Any $(VAR) substring in string for which VAR is not a key in the expansions
  dict will remain in the returned string.
  """

  matches = _xcode_variable_re.findall(string)
  if matches == None:
    return string

  matches.reverse()
  for match in matches:
    (to_replace, variable) = match
    if not variable in expansions:
      continue

    replacement = expansions[variable]
    string = re.sub(re.escape(to_replace), replacement, string)

  return string


def EscapeXCodeArgument(s):
  """We must escape the arguments that we give to XCode so that it knows not to
     split on spaces and to respect backslash and quote literals."""
  s = s.replace('\\', '\\\\')
  s = s.replace('"', '\\"')
  return '"' + s + '"'



def PerformBuild(data, configurations, params):
  options = params['options']

  for build_file, build_file_dict in data.iteritems():
    (build_file_root, build_file_ext) = os.path.splitext(build_file)
    if build_file_ext != '.gyp':
      continue
    xcodeproj_path = build_file_root + options.suffix + '.xcodeproj'
    if options.generator_output:
      xcodeproj_path = os.path.join(options.generator_output, xcodeproj_path)

  for config in configurations:
    arguments = ['xcodebuild', '-project', xcodeproj_path]
    arguments += ['-configuration', config]
    print "Building [%s]: %s" % (config, arguments)
    subprocess.check_call(arguments)


def GenerateOutput(target_list, target_dicts, data, params):
  options = params['options']
  generator_flags = params.get('generator_flags', {})
  parallel_builds = generator_flags.get('xcode_parallel_builds', True)
  serialize_all_tests = \
      generator_flags.get('xcode_serialize_all_test_runs', True)
  project_version = generator_flags.get('xcode_project_version', None)
  skip_excluded_files = \
      not generator_flags.get('xcode_list_excluded_files', True)
  xcode_projects = {}
  for build_file, build_file_dict in data.iteritems():
    (build_file_root, build_file_ext) = os.path.splitext(build_file)
    if build_file_ext != '.gyp':
      continue
    xcodeproj_path = build_file_root + options.suffix + '.xcodeproj'
    if options.generator_output:
      xcodeproj_path = os.path.join(options.generator_output, xcodeproj_path)
    xcp = XcodeProject(build_file, xcodeproj_path, build_file_dict)
    xcode_projects[build_file] = xcp
    pbxp = xcp.project

    if parallel_builds:
      pbxp.SetProperty('attributes',
                       {'BuildIndependentTargetsInParallel': 'YES'})
    if project_version:
      xcp.project_file.SetXcodeVersion(project_version)

    
    if not generator_flags.get('standalone'):
      main_group = pbxp.GetProperty('mainGroup')
      build_group = gyp.xcodeproj_file.PBXGroup({'name': 'Build'})
      main_group.AppendChild(build_group)
      for included_file in build_file_dict['included_files']:
        build_group.AddOrGetFileByPath(included_file, False)

  xcode_targets = {}
  xcode_target_to_target_dict = {}
  for qualified_target in target_list:
    [build_file, target_name, toolset] = \
        gyp.common.ParseQualifiedTarget(qualified_target)

    spec = target_dicts[qualified_target]
    if spec['toolset'] != 'target':
      raise Exception(
          'Multiple toolsets not supported in xcode build (target %s)' %
          qualified_target)
    configuration_names = [spec['default_configuration']]
    for configuration_name in sorted(spec['configurations'].keys()):
      if configuration_name not in configuration_names:
        configuration_names.append(configuration_name)
    xcp = xcode_projects[build_file]
    pbxp = xcp.project

    
    
    xccl = CreateXCConfigurationList(configuration_names)

    
    
    
    
    
    _types = {
      'executable':             'com.apple.product-type.tool',
      'loadable_module':        'com.googlecode.gyp.xcode.bundle',
      'shared_library':         'com.apple.product-type.library.dynamic',
      'static_library':         'com.apple.product-type.library.static',
      'executable+bundle':      'com.apple.product-type.application',
      'loadable_module+bundle': 'com.apple.product-type.bundle',
      'shared_library+bundle':  'com.apple.product-type.framework',
    }

    target_properties = {
      'buildConfigurationList': xccl,
      'name':                   target_name,
    }

    type = spec['type']
    is_bundle = int(spec.get('mac_bundle', 0))
    if type != 'none':
      type_bundle_key = type
      if is_bundle:
        type_bundle_key += '+bundle'
      xctarget_type = gyp.xcodeproj_file.PBXNativeTarget
      try:
        target_properties['productType'] = _types[type_bundle_key]
      except KeyError, e:
        gyp.common.ExceptionAppend(e, "-- unknown product type while "
                                   "writing target %s" % target_name)
        raise
    else:
      xctarget_type = gyp.xcodeproj_file.PBXAggregateTarget
      assert not is_bundle, (
          'mac_bundle targets cannot have type none (target "%s")' %
          target_name)

    target_product_name = spec.get('product_name')
    if target_product_name is not None:
      target_properties['productName'] = target_product_name

    xct = xctarget_type(target_properties, parent=pbxp,
                        force_outdir=spec.get('product_dir'),
                        force_prefix=spec.get('product_prefix'),
                        force_extension=spec.get('product_extension'))
    pbxp.AppendProperty('targets', xct)
    xcode_targets[qualified_target] = xct
    xcode_target_to_target_dict[xct] = spec

    spec_actions = spec.get('actions', [])
    spec_rules = spec.get('rules', [])

    
    
    
    
    
    
    support_xct = None
    if type != 'none' and (spec_actions or spec_rules):
      support_xccl = CreateXCConfigurationList(configuration_names);
      support_target_properties = {
        'buildConfigurationList': support_xccl,
        'name':                   target_name + ' Support',
      }
      if target_product_name:
        support_target_properties['productName'] = \
            target_product_name + ' Support'
      support_xct = \
          gyp.xcodeproj_file.PBXAggregateTarget(support_target_properties,
                                                parent=pbxp)
      pbxp.AppendProperty('targets', support_xct)
      xct.AddDependency(support_xct)
    
    
    xct.support_target = support_xct

    prebuild_index = 0

    
    for action in spec_actions:
      
      
      

      
      message = action.get('message')
      if message:
        message = 'echo note: ' + gyp.common.EncodePOSIXShellArgument(message)
      else:
        message = ''

      
      action_string = gyp.common.EncodePOSIXShellList(action['action'])

      
      
      message_sh = gyp.xcodeproj_file.ConvertVariablesToShellSyntax(message)
      action_string_sh = gyp.xcodeproj_file.ConvertVariablesToShellSyntax(
        action_string)

      script = ''
      
      if message_sh:
        script += message_sh + '\n'
      
      
      script += 'exec ' + action_string_sh + '\nexit 1\n'
      ssbp = gyp.xcodeproj_file.PBXShellScriptBuildPhase({
            'inputPaths': action['inputs'],
            'name': 'Action "' + action['action_name'] + '"',
            'outputPaths': action['outputs'],
            'shellScript': script,
            'showEnvVarsInLog': 0,
          })

      if support_xct:
        support_xct.AppendProperty('buildPhases', ssbp)
      else:
        
        
        
        xct._properties['buildPhases'].insert(prebuild_index, ssbp)
        prebuild_index = prebuild_index + 1

      
      if int(action.get('process_outputs_as_sources', False)):
        for output in action['outputs']:
          AddSourceToTarget(output, type, pbxp, xct)

      if int(action.get('process_outputs_as_mac_bundle_resources', False)):
        for output in action['outputs']:
          AddResourceToTarget(output, pbxp, xct)

    
    
    if is_bundle:
      tgt_mac_bundle_resources = spec.get('mac_bundle_resources', [])
    else:
      tgt_mac_bundle_resources = []

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    rules_by_ext = {}
    for rule in spec_rules:
      rules_by_ext[rule['extension']] = rule

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      

      
      
      
      concrete_outputs_by_rule_source = []

      
      
      
      concrete_outputs_all = []

      
      
      
      
      messages = []
      actions = []

      for rule_source in rule.get('rule_sources', []):
        rule_source_dirname, rule_source_basename = \
            posixpath.split(rule_source)
        (rule_source_root, rule_source_ext) = \
            posixpath.splitext(rule_source_basename)

        
        
        
        rule_input_dict = {
          'INPUT_FILE_BASE':   rule_source_root,
          'INPUT_FILE_SUFFIX': rule_source_ext,
          'INPUT_FILE_NAME':   rule_source_basename,
          'INPUT_FILE_PATH':   rule_source,
          'INPUT_FILE_DIRNAME': rule_source_dirname,
        }

        concrete_outputs_for_this_rule_source = []
        for output in rule.get('outputs', []):
          
          
          
          
          
          concrete_output = ExpandXcodeVariables(output, rule_input_dict)
          concrete_outputs_for_this_rule_source.append(concrete_output)

          
          pbxp.AddOrGetFileInRootGroup(concrete_output)

        concrete_outputs_by_rule_source.append( \
            concrete_outputs_for_this_rule_source)
        concrete_outputs_all.extend(concrete_outputs_for_this_rule_source)

        
        if int(rule.get('process_outputs_as_sources', False)):
          for output in concrete_outputs_for_this_rule_source:
            AddSourceToTarget(output, type, pbxp, xct)

        
        
        was_mac_bundle_resource = rule_source in tgt_mac_bundle_resources
        if was_mac_bundle_resource or \
            int(rule.get('process_outputs_as_mac_bundle_resources', False)):
          for output in concrete_outputs_for_this_rule_source:
            AddResourceToTarget(output, pbxp, xct)

        
        message = rule.get('message')
        if message:
          message = gyp.common.EncodePOSIXShellArgument(message)
          message = ExpandXcodeVariables(message, rule_input_dict)
        messages.append(message)

        
        action_string = gyp.common.EncodePOSIXShellList(rule['action'])

        action = ExpandXcodeVariables(action_string, rule_input_dict)
        actions.append(action)

      if len(concrete_outputs_all) > 0:
        
        
        makefile_name = '%s.make' % re.sub(
            '[^a-zA-Z0-9_]', '_' , '%s_%s' % (target_name, rule['rule_name']))
        makefile_path = os.path.join(xcode_projects[build_file].path,
                                     makefile_name)
        
        
        makefile = open(makefile_path, 'wb')

        
        
        
        
        makefile.write('all: \\\n')
        for concrete_output_index in \
            xrange(0, len(concrete_outputs_by_rule_source)):
          
          
          
          
          
          concrete_output = \
              concrete_outputs_by_rule_source[concrete_output_index][0]
          if concrete_output_index == len(concrete_outputs_by_rule_source) - 1:
            eol = ''
          else:
            eol = ' \\'
          makefile.write('    %s%s\n' % (concrete_output, eol))

        for (rule_source, concrete_outputs, message, action) in \
            zip(rule['rule_sources'], concrete_outputs_by_rule_source,
                messages, actions):
          makefile.write('\n')

          
          
          
          concrete_output_dirs = []
          for concrete_output_index in xrange(0, len(concrete_outputs)):
            concrete_output = concrete_outputs[concrete_output_index]
            if concrete_output_index == 0:
              bol = ''
            else:
              bol = '    '
            makefile.write('%s%s \\\n' % (bol, concrete_output))

            concrete_output_dir = posixpath.dirname(concrete_output)
            if (concrete_output_dir and
                concrete_output_dir not in concrete_output_dirs):
              concrete_output_dirs.append(concrete_output_dir)

          makefile.write('    : \\\n')

          
          
          prerequisites = [rule_source]
          prerequisites.extend(rule.get('inputs', []))
          for prerequisite_index in xrange(0, len(prerequisites)):
            prerequisite = prerequisites[prerequisite_index]
            if prerequisite_index == len(prerequisites) - 1:
              eol = ''
            else:
              eol = ' \\'
            makefile.write('    %s%s\n' % (prerequisite, eol))

          
          
          if len(concrete_output_dirs) > 0:
            makefile.write('\t@mkdir -p "%s"\n' %
                           '" "'.join(concrete_output_dirs))

          
          
          if message:
            
            makefile.write('\t@echo note: %s\n' % message)
          makefile.write('\t%s\n' % action)

        makefile.close()

        
        
        
        

        
        
        
        
        
        
        
        
        
        

        
        
        
        
        
        
        script = \
"""JOB_COUNT="$(/usr/sbin/sysctl -n hw.ncpu)"
if [ "${JOB_COUNT}" -gt 4 ]; then
  JOB_COUNT=4
fi
exec "${DEVELOPER_BIN_DIR}/make" -f "${PROJECT_FILE_PATH}/%s" -j "${JOB_COUNT}"
exit 1
""" % makefile_name
        ssbp = gyp.xcodeproj_file.PBXShellScriptBuildPhase({
              'name': 'Rule "' + rule['rule_name'] + '"',
              'shellScript': script,
              'showEnvVarsInLog': 0,
            })

        if support_xct:
          support_xct.AppendProperty('buildPhases', ssbp)
        else:
          
          
          
          xct._properties['buildPhases'].insert(prebuild_index, ssbp)
          prebuild_index = prebuild_index + 1

      
      
      groups = ['inputs', 'inputs_excluded']
      if skip_excluded_files:
        groups = [x for x in groups if not x.endswith('_excluded')]
      for group in groups:
        for item in rule.get(group, []):
          pbxp.AddOrGetFileInRootGroup(item)

    
    for source in spec.get('sources', []):
      (source_root, source_extension) = posixpath.splitext(source)
      if source_extension[1:] not in rules_by_ext:
        
        
        AddSourceToTarget(source, type, pbxp, xct)
      else:
        pbxp.AddOrGetFileInRootGroup(source)

    
    
    if is_bundle:
      for resource in tgt_mac_bundle_resources:
        (resource_root, resource_extension) = posixpath.splitext(resource)
        if resource_extension[1:] not in rules_by_ext:
          AddResourceToTarget(resource, pbxp, xct)
        else:
          pbxp.AddOrGetFileInRootGroup(resource)

      for header in spec.get('mac_framework_private_headers', []):
        AddHeaderToTarget(header, pbxp, xct, False)

    
    
    if is_bundle or type == 'static_library':
      for header in spec.get('mac_framework_headers', []):
        AddHeaderToTarget(header, pbxp, xct, True)

    
    for copy_group in spec.get('copies', []):
      pbxcp = gyp.xcodeproj_file.PBXCopyFilesBuildPhase({
            'name': 'Copy to ' + copy_group['destination']
          },
          parent=xct)
      dest = copy_group['destination']
      if dest[0] not in ('/', '$'):
        
        dest = '$(SRCROOT)/' + dest
      pbxcp.SetDestination(dest)

      
      
      xct._properties['buildPhases'].insert(prebuild_index, pbxcp)

      for file in copy_group['files']:
        pbxcp.AddFile(file)

    
    if not skip_excluded_files:
      for key in ['sources', 'mac_bundle_resources', 'mac_framework_headers',
                  'mac_framework_private_headers']:
        excluded_key = key + '_excluded'
        for item in spec.get(excluded_key, []):
          pbxp.AddOrGetFileInRootGroup(item)

    
    groups = ['inputs', 'inputs_excluded', 'outputs', 'outputs_excluded']
    if skip_excluded_files:
      groups = [x for x in groups if not x.endswith('_excluded')]
    for action in spec.get('actions', []):
      for group in groups:
        for item in action.get(group, []):
          
          
          if not item.startswith('$(BUILT_PRODUCTS_DIR)/'):
            pbxp.AddOrGetFileInRootGroup(item)

    for postbuild in spec.get('postbuilds', []):
      action_string_sh = gyp.common.EncodePOSIXShellList(postbuild['action'])
      script = 'exec ' + action_string_sh + '\nexit 1\n'

      
      
      
      
      
      ssbp = gyp.xcodeproj_file.PBXShellScriptBuildPhase({
            'inputPaths': ['$(BUILT_PRODUCTS_DIR)/$(EXECUTABLE_PATH)'],
            'name': 'Postbuild "' + postbuild['postbuild_name'] + '"',
            'shellScript': script,
            'showEnvVarsInLog': 0,
          })
      xct.AppendProperty('buildPhases', ssbp)

    
    
    
    
    
    
    
    
    
    

    if 'dependencies' in spec:
      for dependency in spec['dependencies']:
        xct.AddDependency(xcode_targets[dependency])
        
        
        if support_xct:
          support_xct.AddDependency(xcode_targets[dependency])

    if 'libraries' in spec:
      for library in spec['libraries']:
        xct.FrameworksPhase().AddFile(library)
        
        
        library_dir = posixpath.dirname(library)
        if library_dir not in xcode_standard_library_dirs and (
            not xct.HasBuildSetting(_library_search_paths_var) or
            library_dir not in xct.GetBuildSetting(_library_search_paths_var)):
          xct.AppendBuildSetting(_library_search_paths_var, library_dir)

    for configuration_name in configuration_names:
      configuration = spec['configurations'][configuration_name]
      xcbc = xct.ConfigurationNamed(configuration_name)
      for include_dir in configuration.get('mac_framework_dirs', []):
        xcbc.AppendBuildSetting('FRAMEWORK_SEARCH_PATHS', include_dir)
      for include_dir in configuration.get('include_dirs', []):
        xcbc.AppendBuildSetting('HEADER_SEARCH_PATHS', include_dir)
      if 'defines' in configuration:
        for define in configuration['defines']:
          set_define = EscapeXCodeArgument(define)
          xcbc.AppendBuildSetting('GCC_PREPROCESSOR_DEFINITIONS', set_define)
      if 'xcode_settings' in configuration:
        for xck, xcv in configuration['xcode_settings'].iteritems():
          xcbc.SetBuildSetting(xck, xcv)
      if 'xcode_config_file' in configuration:
        config_ref = pbxp.AddOrGetFileInRootGroup(
            configuration['xcode_config_file'])
        xcbc.SetBaseConfiguration(config_ref)

  build_files = []
  for build_file, build_file_dict in data.iteritems():
    if build_file.endswith('.gyp'):
      build_files.append(build_file)

  for build_file in build_files:
    xcode_projects[build_file].Finalize1(xcode_targets, serialize_all_tests)

  for build_file in build_files:
    xcode_projects[build_file].Finalize2(xcode_targets,
                                         xcode_target_to_target_dict)

  for build_file in build_files:
    xcode_projects[build_file].Write()
