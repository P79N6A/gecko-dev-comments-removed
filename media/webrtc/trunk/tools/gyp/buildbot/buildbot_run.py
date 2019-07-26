





"""Argument-less script to select what to run on the buildbots."""


import os
import shutil
import subprocess
import sys


if sys.platform in ['win32', 'cygwin']:
  EXE_SUFFIX = '.exe'
else:
  EXE_SUFFIX = ''


BUILDBOT_DIR = os.path.dirname(os.path.abspath(__file__))
TRUNK_DIR = os.path.dirname(BUILDBOT_DIR)
ROOT_DIR = os.path.dirname(TRUNK_DIR)
OUT_DIR = os.path.join(TRUNK_DIR, 'out')


def GypTestFormat(title, format=None, msvs_version=None):
  """Run the gyp tests for a given format, emitting annotator tags.

  See annotator docs at:
    https://sites.google.com/a/chromium.org/dev/developers/testing/chromium-build-infrastructure/buildbot-annotations
  Args:
    format: gyp format to test.
  Returns:
    0 for sucesss, 1 for failure.
  """
  if not format:
    format = title

  print '@@@BUILD_STEP ' + title + '@@@'
  sys.stdout.flush()
  env = os.environ.copy()
  
  
  if format == 'ninja':
    env['NOGOLD'] = '1'
  if msvs_version:
    env['GYP_MSVS_VERSION'] = msvs_version
  retcode = subprocess.call(' '.join(
      [sys.executable, 'trunk/gyptest.py',
       '--all',
       '--passed',
       '--format', format,
       '--chdir', 'trunk',
       '--path', '../scons']),
      cwd=ROOT_DIR, env=env, shell=True)
  if retcode:
    
    print '@@@STEP_FAILURE@@@'
    return 1
  return 0


def GypBuild():
  
  print '@@@BUILD_STEP cleanup@@@'
  print 'Removing %s...' % OUT_DIR
  shutil.rmtree(OUT_DIR, ignore_errors=True)
  print 'Done.'

  retcode = 0
  if sys.platform.startswith('linux'):
    retcode += GypTestFormat('ninja')
    retcode += GypTestFormat('scons')
    retcode += GypTestFormat('make')
  elif sys.platform == 'darwin':
    retcode += GypTestFormat('ninja')
    retcode += GypTestFormat('xcode')
    retcode += GypTestFormat('make')
  elif sys.platform == 'win32':
    retcode += GypTestFormat('ninja')
    retcode += GypTestFormat('msvs-2008', format='msvs', msvs_version='2008')
    if os.environ['BUILDBOT_BUILDERNAME'] == 'gyp-win64':
      retcode += GypTestFormat('msvs-2010', format='msvs', msvs_version='2010')
  else:
    raise Exception('Unknown platform')
  if retcode:
    
    
    
    
    print '@@@BUILD_STEP failures@@@'
    sys.exit(retcode)


if __name__ == '__main__':
  GypBuild()
