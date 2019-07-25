








"""
Runs Coverity Static Analysis on a build of WebRTC.

This script is a modified copy of Chromium's tools/coverity/coverity.py
Changes made:
 * Replaced deprecated switches for cov-commit-defects command:
   * Using --host instead of --remote
   * Using --stream instead of --product
   * Removed --cxx (now default enabled)
 * Changed cleaning of output path, since WebRTC's out dir is located directly
   in trunk/
 * Updated some default constants.

The script runs on all WebRTC supported platforms.

On Windows, this script should be run in a Visual Studio Command Prompt, so
that the INCLUDE, LIB, and PATH environment variables are set properly for
Visual Studio.

Usage examples:
  coverity.py
  coverity.py --dry-run
  coverity.py --target=debug
  %comspec% /c ""C:\Program Files\Microsoft Visual Studio 8\VC\vcvarsall.bat"
      x86 && C:\Python24\python.exe C:\coverity.py"

For a full list of options, pass the '--help' switch.

See http://support.microsoft.com/kb/308569 for running this script as a
Scheduled Task on Windows XP.
"""

import optparse
import os
import os.path
import shutil
import subprocess
import sys
import time









WEBRTC_SOURCE_DIR = 'C:\\webrtc.latest'


WEBRTC_SOLUTION_FILE = 'webrtc.sln'


WEBRTC_SOLUTION_DIR = 'build'

COVERITY_BIN_DIR = 'C:\\coverity-integrity-center\\static-analysis\\bin'

COVERITY_INTERMEDIATE_DIR = 'C:\\coverity\\cvbuild\\cr_int'

COVERITY_ANALYZE_OPTIONS = ('--security --concurrency '
                            '--enable ATOMICITY '
                            '--enable MISSING_LOCK '
                            '--enable DELETE_VOID '
                            '--checker-option PASS_BY_VALUE:size_threshold:16 '
                            '--checker-option '
                            'USE_AFTER_FREE:allow_simple_use:false '
                            '--enable-constraint-fpp '
                            '--enable-callgraph-metrics')


COVERITY_REMOTE = 'localhost'

COVERITY_PORT = '8080'

COVERITY_STREAM = 'trunk'

COVERITY_TARGET = 'Windows'

COVERITY_USER = 'coverityanalyzer'



LOCK_FILE = 'coverity.lock'


def _ReadPassword(pwfilename):
  """Reads the coverity password in from a file where it was stashed"""
  pwfile = open(pwfilename, 'r')
  password = pwfile.readline()
  pwfile.close()
  return password.rstrip()


def _RunCommand(cmd, dry_run, shell=False, echo_cmd=True):
  """Runs the command if dry_run is false, otherwise just prints the command."""
  if echo_cmd:
    print cmd
  if not dry_run:
    return subprocess.call(cmd, shell=shell)
  else:
    return 0


def _ReleaseLock(lock_file, lock_filename):
  """Removes the lockfile. Function-ized so we can bail from anywhere"""
  os.close(lock_file)
  os.remove(lock_filename)


def run_coverity(options, args):
  """Runs all the selected tests for the given build type and target."""
  
  
  lock_filename = os.path.join(options.source_dir, LOCK_FILE)
  try:
    lock_file = os.open(lock_filename,
                        os.O_CREAT | os.O_EXCL | os.O_TRUNC | os.O_RDWR)
  except OSError, err:
    print 'Failed to open lock file:\n  ' + str(err)
    return 1

  
  os.write(lock_file, str(os.getpid()))

  options.target = options.target.title()

  start_time = time.time()

  print 'Change directory to ' + options.source_dir
  os.chdir(options.source_dir)

  
  
  
  coverity_password = _ReadPassword(options.coverity_password_file)

  cmd = 'gclient sync'
  gclient_exit = _RunCommand(cmd, options.dry_run, shell=True)
  if gclient_exit != 0:
    print 'gclient aborted with status %s' % gclient_exit
    _ReleaseLock(lock_file, lock_filename)
    return 1

  print 'Elapsed time: %ds' % (time.time() - start_time)

  
  if sys.platform.startswith('linux'):
    rm_path = os.path.join(options.source_dir,'out',options.target)
  elif sys.platform == 'win32':
    rm_path = os.path.join(options.source_dir,options.solution_dir,
                           options.target)
  elif sys.platform == 'darwin':
    rm_path = os.path.join(options.source_dir,'xcodebuild')
  else:
    print 'Platform "%s" unrecognized, aborting' % sys.platform
    _ReleaseLock(lock_file, lock_filename)
    return 1

  if options.dry_run:
    print 'shutil.rmtree(%s)' % repr(rm_path)
  else:
    shutil.rmtree(rm_path,True)

  if options.preserve_intermediate_dir:
      print 'Preserving intermediate directory.'
  else:
    if options.dry_run:
      print 'shutil.rmtree(%s)' % repr(options.coverity_intermediate_dir)
      print 'os.mkdir(%s)' % repr(options.coverity_intermediate_dir)
    else:
      shutil.rmtree(options.coverity_intermediate_dir,True)
      os.mkdir(options.coverity_intermediate_dir)

  print 'Elapsed time: %ds' % (time.time() - start_time)

  use_shell_during_make = False
  if sys.platform.startswith('linux'):
    use_shell_during_make = True
    _RunCommand('pwd', options.dry_run, shell=True)
    cmd = '%s/cov-build --dir %s make BUILDTYPE=%s All' % (
      options.coverity_bin_dir, options.coverity_intermediate_dir,
      options.target)
  elif sys.platform == 'win32':
    cmd = ('%s\\cov-build.exe --dir %s devenv.com %s\\%s /build %s '
           '/project webrtc.vcproj') % (
      options.coverity_bin_dir, options.coverity_intermediate_dir,
      options.source_dir, options.solution_file, options.target)
  elif sys.platform == 'darwin':
    use_shell_during_make = True
    _RunCommand('pwd', options.dry_run, shell=True)
    cmd = ('%s/cov-build --dir %s xcodebuild -project webrtc.xcodeproj '
           '-configuration %s -target All') % (
      options.coverity_bin_dir, options.coverity_intermediate_dir,
      options.target)


  _RunCommand(cmd, options.dry_run, shell=use_shell_during_make)
  print 'Elapsed time: %ds' % (time.time() - start_time)

  cov_analyze_exe = os.path.join(options.coverity_bin_dir,'cov-analyze')
  cmd = '%s --dir %s %s' % (cov_analyze_exe,
                            options.coverity_intermediate_dir,
                            options.coverity_analyze_options)
  _RunCommand(cmd, options.dry_run, shell=use_shell_during_make)
  print 'Elapsed time: %ds' % (time.time() - start_time)

  cov_commit_exe = os.path.join(options.coverity_bin_dir,'cov-commit-defects')

  
  
  
  
  
  coverity_target = options.coverity_target
  if sys.platform != 'win32':
    coverity_target = '"%s"' % coverity_target

  cmd = ('%s --dir %s --host %s --port %s '
         '--stream %s '
         '--target %s '
         '--user %s '
         '--password %s') % (cov_commit_exe,
                             options.coverity_intermediate_dir,
                             options.coverity_dbhost,
                             options.coverity_port,
                             options.coverity_stream,
                             coverity_target,
                             options.coverity_user,
                             coverity_password)
  
  print 'Commiting defects to Coverity Integrity Manager server...'
  _RunCommand(cmd, options.dry_run, shell=use_shell_during_make, echo_cmd=False)

  print 'Completed! Total time: %ds' % (time.time() - start_time)

  _ReleaseLock(lock_file, lock_filename)

  return 0


def main():
  option_parser = optparse.OptionParser()
  option_parser.add_option('', '--dry-run', action='store_true', default=False,
                           help='print but don\'t run the commands')

  option_parser.add_option('', '--target', default='Release',
                           help='build target (Debug or Release)')

  option_parser.add_option('', '--source-dir', dest='source_dir',
                           help='full path to directory ABOVE "src"',
                           default=WEBRTC_SOURCE_DIR)

  option_parser.add_option('', '--solution-file', dest='solution_file',
                           help='filename of solution file to build (Win only)',
                           default=WEBRTC_SOLUTION_FILE)

  option_parser.add_option('', '--solution-dir', dest='solution_dir',
                           help='build directory for the solution (Win only)',
                           default=WEBRTC_SOLUTION_DIR)

  option_parser.add_option('', '--coverity-bin-dir', dest='coverity_bin_dir',
                           default=COVERITY_BIN_DIR)

  option_parser.add_option('', '--coverity-intermediate-dir',
                           dest='coverity_intermediate_dir',
                           default=COVERITY_INTERMEDIATE_DIR)

  option_parser.add_option('', '--coverity-analyze-options',
                           dest='coverity_analyze_options',
                           help=('all cov-analyze options, e.g. "%s"'
                                 % COVERITY_ANALYZE_OPTIONS),
                           default=COVERITY_ANALYZE_OPTIONS)

  option_parser.add_option('', '--coverity-db-host',
                           dest='coverity_dbhost',
                           help=('coverity defect db server hostname, e.g. %s'
                                 % COVERITY_REMOTE),
                           default=COVERITY_REMOTE)

  option_parser.add_option('', '--coverity-db-port', dest='coverity_port',
                           help=('port # of coverity web/db server, e.g. %s'
                                 % COVERITY_PORT),
                           default=COVERITY_PORT)

  option_parser.add_option('', '--coverity-stream', dest='coverity_stream',
                           help=('Name of stream reported to Coverity, e.g. %s'
                                 % COVERITY_STREAM),
                           default=COVERITY_STREAM)

  option_parser.add_option('', '--coverity-target', dest='coverity_target',
                           help='Platform Target reported to coverity',
                           default=COVERITY_TARGET)

  option_parser.add_option('', '--coverity-user', dest='coverity_user',
                           help='Username used to log into coverity',
                           default=COVERITY_USER)

  option_parser.add_option('', '--coverity-password-file',
                           dest='coverity_password_file',
                           help='file containing the coverity password',
                           default='coverity-password')

  helpmsg = ('By default, the intermediate dir is emptied before analysis. '
             'This switch disables that behavior.')
  option_parser.add_option('', '--preserve-intermediate-dir',
                           action='store_true', help=helpmsg,
                           default=False)

  options, args = option_parser.parse_args()
  return run_coverity(options, args)


if '__main__' == __name__:
  sys.exit(main())
