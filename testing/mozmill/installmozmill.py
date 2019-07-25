






































"""
install mozmill and its dependencies
"""

import os
import sys
from optparse import OptionParser
from subprocess import call



def is_windows():
  return sys.platform.startswith('win')

def esc(path):
  """quote and escape a path for cross-platform use"""
  return '"%s"' % repr(path)[1:-1]

def scripts_path(virtual_env):
  """path to scripts directory"""
  if is_windows():
    return os.path.join(virtual_env, 'Scripts')
  return os.path.join(virtual_env, 'bin')

def python_script_path(virtual_env, script_name):
  """path to a python script in a virtualenv"""
  scripts_dir = scripts_path(virtual_env)
  if is_windows():
    script_name = script_name + '-script.py'
  return os.path.join(scripts_dir, script_name)

def entry_point_path(virtual_env, entry_point):
  path = os.path.join(scripts_path(virtual_env), entry_point)
  if is_windows():
    path += '.exe'
  return path



def main(args=None):
  """command line front-end function"""

  
  args = args or sys.argv[1:]
  usage = "Usage: %prog [options] [destination]"
  parser = OptionParser(usage=usage)
  parser.add_option('--develop', dest='develop',
                    action='store_true', default=False,
                    help='setup in development mode')
  options, args = parser.parse_args(args)

  
  print 'Python: %s' % sys.version

  
  source=os.path.abspath(os.path.dirname(__file__))

  
  if not len(args):
    destination = source
  elif len(args) == 1:
    destination = os.path.abspath(args[0])
  else:
    parser.print_usage()
    parser.exit(1)

  os.chdir(source)

  
  if not os.path.exists('virtualenv'):
    print "File not found: virtualenv"
    sys.exit(1)
  PACKAGES_FILE = 'PACKAGES'
  if not os.path.exists(PACKAGES_FILE) and destination != source:
      PACKAGES_FILE = os.path.join(destination, PACKAGES_FILE)
  if not os.path.exists(PACKAGES_FILE):
    print "File not found: PACKAGES"

  
  PACKAGES=file(PACKAGES_FILE).read().split()
  assert PACKAGES
  
  
  env = os.environ.copy()
  env.pop('PYTHONHOME', None)
  returncode = call([sys.executable, os.path.join('virtualenv', 'virtualenv.py'), destination], env=env)
  if returncode:
    print 'Failure to install virtualenv'
    sys.exit(returncode)
  if options.develop:
    python = entry_point_path(destination, 'python')
    for package in PACKAGES:
      oldcwd = os.getcwd()
      os.chdir(package)
      returncode = call([python, 'setup.py', 'develop'])
      os.chdir(oldcwd)
      if returncode:
        break
  else:
    pip = entry_point_path(destination, 'pip')
    returncode = call([pip, 'install'] + PACKAGES, env=env)

  if returncode:
    print 'Failure to install packages'
    sys.exit(returncode)

  
  template = """#!/bin/bash
unset PYTHONHOME
%(PYTHON)s %(MOZMILL)s $@
"""
  variables = {'PYTHON': esc(entry_point_path(destination, 'python')) }
  for script in 'mozmill', 'mozmill-restart':
    path = os.path.join(destination, script + '.sh')
    f = file(path, 'w')
    variables['MOZMILL'] = esc(python_script_path(destination, script))
    print >> f, template % variables
    f.close()
    if not is_windows():
      os.chmod(path, 0755)

if __name__ == '__main__':
  main()
