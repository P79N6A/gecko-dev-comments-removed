





import copy
import gyp.input
import optparse
import os.path
import re
import shlex
import sys
import traceback
from gyp.common import GypError


debug = {}


DEBUG_GENERAL = 'general'
DEBUG_VARIABLES = 'variables'
DEBUG_INCLUDES = 'includes'


def DebugOutput(mode, message):
  if 'all' in gyp.debug.keys() or mode in gyp.debug.keys():
    ctx = ('unknown', 0, 'unknown')
    try:
      f = traceback.extract_stack(limit=2)
      if f:
        ctx = f[0][:3]
    except:
      pass
    print '%s:%s:%d:%s %s' % (mode.upper(), os.path.basename(ctx[0]),
                              ctx[1], ctx[2], message)

def FindBuildFiles():
  extension = '.gyp'
  files = os.listdir(os.getcwd())
  build_files = []
  for file in files:
    if file.endswith(extension):
      build_files.append(file)
  return build_files


def Load(build_files, format, default_variables={},
         includes=[], depth='.', params=None, check=False,
         circular_check=True):
  """
  Loads one or more specified build files.
  default_variables and includes will be copied before use.
  Returns the generator for the specified format and the
  data returned by loading the specified build files.
  """
  if params is None:
    params = {}

  flavor = None
  if '-' in format:
    format, params['flavor'] = format.split('-', 1)

  default_variables = copy.copy(default_variables)

  
  
  
  default_variables['GENERATOR'] = format

  
  default_variables['PYTHON'] = sys.executable

  
  
  if format.endswith('.py'):
    generator_name = os.path.splitext(format)[0]
    path, generator_name = os.path.split(generator_name)

    
    
    
    
    path = os.path.abspath(path)
    if path not in sys.path:
      sys.path.insert(0, path)
  else:
    generator_name = 'gyp.generator.' + format

  
  
  generator = __import__(generator_name, globals(), locals(), generator_name)
  for (key, val) in generator.generator_default_variables.items():
    default_variables.setdefault(key, val)

  
  
  if getattr(generator, 'CalculateVariables', None):
    generator.CalculateVariables(default_variables, params)

  
  
  if getattr(generator, 'CalculateGeneratorInputInfo', None):
    generator.CalculateGeneratorInputInfo(params)

  
  
  
  generator_input_info = {
    'generator_wants_absolute_build_file_paths':
        getattr(generator, 'generator_wants_absolute_build_file_paths', False),
    'generator_handles_variants':
        getattr(generator, 'generator_handles_variants', False),
    'non_configuration_keys':
        getattr(generator, 'generator_additional_non_configuration_keys', []),
    'path_sections':
        getattr(generator, 'generator_additional_path_sections', []),
    'extra_sources_for_rules':
        getattr(generator, 'generator_extra_sources_for_rules', []),
    'generator_supports_multiple_toolsets':
        getattr(generator, 'generator_supports_multiple_toolsets', False),
    'generator_wants_static_library_dependencies_adjusted':
        getattr(generator,
                'generator_wants_static_library_dependencies_adjusted', True),
    'generator_wants_sorted_dependencies':
        getattr(generator, 'generator_wants_sorted_dependencies', False),
  }

  
  result = gyp.input.Load(build_files, default_variables, includes[:],
                          depth, generator_input_info, check, circular_check,
                          params['parallel'])
  return [generator] + result

def NameValueListToDict(name_value_list):
  """
  Takes an array of strings of the form 'NAME=VALUE' and creates a dictionary
  of the pairs.  If a string is simply NAME, then the value in the dictionary
  is set to True.  If VALUE can be converted to an integer, it is.
  """
  result = { }
  for item in name_value_list:
    tokens = item.split('=', 1)
    if len(tokens) == 2:
      
      try:
        token_value = int(tokens[1])
      except ValueError:
        token_value = tokens[1]
      
      result[tokens[0]] = token_value
    else:
      
      result[tokens[0]] = True
  return result

def ShlexEnv(env_name):
  flags = os.environ.get(env_name, [])
  if flags:
    flags = shlex.split(flags)
  return flags

def FormatOpt(opt, value):
  if opt.startswith('--'):
    return '%s=%s' % (opt, value)
  return opt + value

def RegenerateAppendFlag(flag, values, predicate, env_name, options):
  """Regenerate a list of command line flags, for an option of action='append'.

  The |env_name|, if given, is checked in the environment and used to generate
  an initial list of options, then the options that were specified on the
  command line (given in |values|) are appended.  This matches the handling of
  environment variables and command line flags where command line flags override
  the environment, while not requiring the environment to be set when the flags
  are used again.
  """
  flags = []
  if options.use_environment and env_name:
    for flag_value in ShlexEnv(env_name):
      value = FormatOpt(flag, predicate(flag_value))
      if value in flags:
        flags.remove(value)
      flags.append(value)
  if values:
    for flag_value in values:
      flags.append(FormatOpt(flag, predicate(flag_value)))
  return flags

def RegenerateFlags(options):
  """Given a parsed options object, and taking the environment variables into
  account, returns a list of flags that should regenerate an equivalent options
  object (even in the absence of the environment variables.)

  Any path options will be normalized relative to depth.

  The format flag is not included, as it is assumed the calling generator will
  set that as appropriate.
  """
  def FixPath(path):
    path = gyp.common.FixIfRelativePath(path, options.depth)
    if not path:
      return os.path.curdir
    return path

  def Noop(value):
    return value

  
  
  flags = ['--ignore-environment']
  for name, metadata in options._regeneration_metadata.iteritems():
    opt = metadata['opt']
    value = getattr(options, name)
    value_predicate = metadata['type'] == 'path' and FixPath or Noop
    action = metadata['action']
    env_name = metadata['env_name']
    if action == 'append':
      flags.extend(RegenerateAppendFlag(opt, value, value_predicate,
                                        env_name, options))
    elif action in ('store', None):  
      if value:
        flags.append(FormatOpt(opt, value_predicate(value)))
      elif options.use_environment and env_name and os.environ.get(env_name):
        flags.append(FormatOpt(opt, value_predicate(os.environ.get(env_name))))
    elif action in ('store_true', 'store_false'):
      if ((action == 'store_true' and value) or
          (action == 'store_false' and not value)):
        flags.append(opt)
      elif options.use_environment and env_name:
        print >>sys.stderr, ('Warning: environment regeneration unimplemented '
                             'for %s flag %r env_name %r' % (action, opt,
                                                             env_name))
    else:
      print >>sys.stderr, ('Warning: regeneration unimplemented for action %r '
                           'flag %r' % (action, opt))

  return flags

class RegeneratableOptionParser(optparse.OptionParser):
  def __init__(self):
    self.__regeneratable_options = {}
    optparse.OptionParser.__init__(self)

  def add_option(self, *args, **kw):
    """Add an option to the parser.

    This accepts the same arguments as OptionParser.add_option, plus the
    following:
      regenerate: can be set to False to prevent this option from being included
                  in regeneration.
      env_name: name of environment variable that additional values for this
                option come from.
      type: adds type='path', to tell the regenerator that the values of
            this option need to be made relative to options.depth
    """
    env_name = kw.pop('env_name', None)
    if 'dest' in kw and kw.pop('regenerate', True):
      dest = kw['dest']

      
      
      type = kw.get('type')
      if type == 'path':
        kw['type'] = 'string'

      self.__regeneratable_options[dest] = {
          'action': kw.get('action'),
          'type': type,
          'env_name': env_name,
          'opt': args[0],
        }

    optparse.OptionParser.add_option(self, *args, **kw)

  def parse_args(self, *args):
    values, args = optparse.OptionParser.parse_args(self, *args)
    values._regeneration_metadata = self.__regeneratable_options
    return values, args

def gyp_main(args):
  my_name = os.path.basename(sys.argv[0])

  parser = RegeneratableOptionParser()
  usage = 'usage: %s [options ...] [build_file ...]'
  parser.set_usage(usage.replace('%s', '%prog'))
  parser.add_option('-D', dest='defines', action='append', metavar='VAR=VAL',
                    env_name='GYP_DEFINES',
                    help='sets variable VAR to value VAL')
  parser.add_option('-f', '--format', dest='formats', action='append',
                    env_name='GYP_GENERATORS', regenerate=False,
                    help='output formats to generate')
  parser.add_option('--msvs-version', dest='msvs_version',
                    regenerate=False,
                    help='Deprecated; use -G msvs_version=MSVS_VERSION instead')
  parser.add_option('-I', '--include', dest='includes', action='append',
                    metavar='INCLUDE', type='path',
                    help='files to include in all loaded .gyp files')
  parser.add_option('--depth', dest='depth', metavar='PATH', type='path',
                    help='set DEPTH gyp variable to a relative path to PATH')
  parser.add_option('-d', '--debug', dest='debug', metavar='DEBUGMODE',
                    action='append', default=[], help='turn on a debugging '
                    'mode for debugging GYP.  Supported modes are "variables", '
                    '"includes" and "general" or "all" for all of them.')
  parser.add_option('-S', '--suffix', dest='suffix', default='',
                    help='suffix to add to generated files')
  parser.add_option('-G', dest='generator_flags', action='append', default=[],
                    metavar='FLAG=VAL', env_name='GYP_GENERATOR_FLAGS',
                    help='sets generator flag FLAG to VAL')
  parser.add_option('--generator-output', dest='generator_output',
                    action='store', default=None, metavar='DIR', type='path',
                    env_name='GYP_GENERATOR_OUTPUT',
                    help='puts generated build files under DIR')
  parser.add_option('--ignore-environment', dest='use_environment',
                    action='store_false', default=True, regenerate=False,
                    help='do not read options from environment variables')
  parser.add_option('--check', dest='check', action='store_true',
                    help='check format of gyp files')
  parser.add_option('--parallel', action='store_true',
                    env_name='GYP_PARALLEL',
                    help='Use multiprocessing for speed (experimental)')
  parser.add_option('--toplevel-dir', dest='toplevel_dir', action='store',
                    default=None, metavar='DIR', type='path',
                    help='directory to use as the root of the source tree')
  parser.add_option('--build', dest='configs', action='append',
                    help='configuration for build after project generation')
  
  
  
  
  
  
  
  parser.add_option('--no-circular-check', dest='circular_check',
                    action='store_false', default=True, regenerate=False,
                    help="don't check for circular relationships between files")

  
  home_vars = ['HOME']
  if sys.platform in ('cygwin', 'win32'):
    home_vars.append('USERPROFILE')
  home = None
  home_dot_gyp = None
  for home_var in home_vars:
    home = os.getenv(home_var)
    if home != None:
      home_dot_gyp = os.path.join(home, '.gyp')
      if not os.path.exists(home_dot_gyp):
        home_dot_gyp = None
      else:
        break

  

  options, build_files_arg = parser.parse_args(args)
  build_files = build_files_arg

  if not options.formats:
    
    generate_formats = []
    if options.use_environment:
      generate_formats = os.environ.get('GYP_GENERATORS', [])
    if generate_formats:
      generate_formats = re.split('[\s,]', generate_formats)
    if generate_formats:
      options.formats = generate_formats
    else:
      
      if sys.platform == 'darwin':
        options.formats = ['xcode']
      elif sys.platform in ('win32', 'cygwin'):
        options.formats = ['msvs']
      else:
        options.formats = ['make']

  if not options.generator_output and options.use_environment:
    g_o = os.environ.get('GYP_GENERATOR_OUTPUT')
    if g_o:
      options.generator_output = g_o

  if not options.parallel and options.use_environment:
    options.parallel = bool(os.environ.get('GYP_PARALLEL'))

  for mode in options.debug:
    gyp.debug[mode] = 1

  
  if DEBUG_GENERAL in gyp.debug.keys():
    DebugOutput(DEBUG_GENERAL, 'running with these options:')
    for option, value in sorted(options.__dict__.items()):
      if option[0] == '_':
        continue
      if isinstance(value, basestring):
        DebugOutput(DEBUG_GENERAL, "  %s: '%s'" % (option, value))
      else:
        DebugOutput(DEBUG_GENERAL, "  %s: %s" % (option, str(value)))

  if not build_files:
    build_files = FindBuildFiles()
  if not build_files:
    raise GypError((usage + '\n\n%s: error: no build_file') %
                   (my_name, my_name))

  
  
  
  
  
  
  if not options.depth:
    for build_file in build_files:
      build_file_dir = os.path.abspath(os.path.dirname(build_file))
      build_file_dir_components = build_file_dir.split(os.path.sep)
      components_len = len(build_file_dir_components)
      for index in xrange(components_len - 1, -1, -1):
        if build_file_dir_components[index] == 'src':
          options.depth = os.path.sep.join(build_file_dir_components)
          break
        del build_file_dir_components[index]

      
      
      if options.depth:
        break

    if not options.depth:
      raise GypError('Could not automatically locate src directory.  This is'
                     'a temporary Chromium feature that will be removed.  Use'
                     '--depth as a workaround.')

  
  
  if not options.toplevel_dir:
    options.toplevel_dir = options.depth

  
  
  
  cmdline_default_variables = {}
  defines = []
  if options.use_environment:
    defines += ShlexEnv('GYP_DEFINES')
  if options.defines:
    defines += options.defines
  cmdline_default_variables = NameValueListToDict(defines)
  if DEBUG_GENERAL in gyp.debug.keys():
    DebugOutput(DEBUG_GENERAL,
                "cmdline_default_variables: %s" % cmdline_default_variables)

  
  includes = []

  
  
  if home_dot_gyp != None:
    default_include = os.path.join(home_dot_gyp, 'include.gypi')
    if os.path.exists(default_include):
      print 'Using overrides found in ' + default_include
      includes.append(default_include)

  
  if options.includes:
    includes.extend(options.includes)

  
  
  gen_flags = []
  if options.use_environment:
    gen_flags += ShlexEnv('GYP_GENERATOR_FLAGS')
  if options.generator_flags:
    gen_flags += options.generator_flags
  generator_flags = NameValueListToDict(gen_flags)
  if DEBUG_GENERAL in gyp.debug.keys():
    DebugOutput(DEBUG_GENERAL, "generator_flags: %s" % generator_flags)

  
  
  if options.msvs_version:
    print >>sys.stderr, \
      'DEPRECATED: Use generator flag (-G msvs_version=' + \
      options.msvs_version + ') instead of --msvs-version=' + \
      options.msvs_version
    generator_flags['msvs_version'] = options.msvs_version

  
  
  for format in set(options.formats):
    params = {'options': options,
              'build_files': build_files,
              'generator_flags': generator_flags,
              'cwd': os.getcwd(),
              'build_files_arg': build_files_arg,
              'gyp_binary': sys.argv[0],
              'home_dot_gyp': home_dot_gyp,
              'parallel': options.parallel}

    
    [generator, flat_list, targets, data] = Load(build_files, format,
                                                 cmdline_default_variables,
                                                 includes, options.depth,
                                                 params, options.check,
                                                 options.circular_check)

    
    
    
    
    
    
    
    generator.GenerateOutput(flat_list, targets, data, params)

    if options.configs:
      valid_configs = targets[flat_list[0]]['configurations'].keys()
      for conf in options.configs:
        if conf not in valid_configs:
          raise GypError('Invalid config specified via --build: %s' % conf)
      generator.PerformBuild(data, options.configs, params)

  
  return 0


def main(args):
  try:
    return gyp_main(args)
  except GypError, e:
    sys.stderr.write("gyp: %s\n" % e)
    return 1

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
