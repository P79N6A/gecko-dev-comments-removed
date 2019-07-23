





































"""
Runs the reftest test harness.
"""

import sys, shutil, os, os.path
SCRIPT_DIRECTORY = os.path.abspath(os.path.realpath(os.path.dirname(sys.argv[0])))
sys.path.append(SCRIPT_DIRECTORY)
import automation
from optparse import OptionParser
from tempfile import mkdtemp

oldcwd = os.getcwd()
os.chdir(SCRIPT_DIRECTORY)

def getFullPath(path):
  "Get an absolute path relative to oldcwd."
  return os.path.normpath(os.path.join(oldcwd, os.path.expanduser(path)))

def createReftestProfile(profileDir):
  "Sets up a clean profile for reftest."

  
  shutil.rmtree(profileDir, True)
  os.mkdir(profileDir)
  
  prefsFile = open(os.path.join(profileDir, "user.js"), "w")
  prefsFile.write("""user_pref("browser.dom.window.dump.enabled", true);
""")
  prefsFile.close()
  
  profileExtensionsPath = os.path.join(profileDir, "extensions")
  os.mkdir(profileExtensionsPath)
  reftestExtensionPath = os.path.join(SCRIPT_DIRECTORY, "reftest")
  extFile = open(os.path.join(profileExtensionsPath, "reftest@mozilla.org"), "w")
  extFile.write(reftestExtensionPath)
  extFile.close()

def main():
  parser = OptionParser()
  parser.add_option("--appname",
                    action = "store", type = "string", dest = "app",
                    default = os.path.join(SCRIPT_DIRECTORY, automation.DEFAULT_APP),
                    help = "absolute path to application, overriding default")
  parser.add_option("--extra-profile-file",
                    action = "append", dest = "extraProfileFiles",
                    default = [],
                    help = "copy specified files/dirs to testing profile")
  options, args = parser.parse_args()
  
  if len(args) != 1:
    print >>sys.stderr, "No reftest.list specified."
    sys.exit(1)

  options.app = getFullPath(options.app)
  if not os.path.exists(options.app):
    print """Error: Path %(app)s doesn't exist.
Are you executing $objdir/_tests/reftest/runreftest.py?""" \
        % {"app": options.app}
    sys.exit(1)

  profileDir = None
  try:
    profileDir = mkdtemp()
    createReftestProfile(profileDir)
    copyExtraFilesToProfile(options, profileDir)

    
    browserEnv = dict(os.environ)

    
    
    browserEnv["NO_EM_RESTART"] = "1"
    browserEnv["XPCOM_DEBUG_BREAK"] = "warn"
    appDir = os.path.dirname(options.app)
    if automation.UNIXISH:
      browserEnv["LD_LIBRARY_PATH"] = appDir
      browserEnv["MOZILLA_FIVE_HOME"] = appDir
      browserEnv["GNOME_DISABLE_CRASH_DIALOG"] = "1"

    
    
    (status, start) = automation.runApp(None, browserEnv, options.app,
                                        profileDir,
                                        extraArgs = ["-silent"])
    
    reftestlist = getFullPath(args[0])
    (status, start) = automation.runApp(None, browserEnv, options.app,
                                        profileDir,
                                        extraArgs = ["-reftest", reftestlist])
  finally:
    if profileDir is not None:
      shutil.rmtree(profileDir)
  sys.exit(status)

def copyExtraFilesToProfile(options, profileDir):
  "Copy extra files or dirs specified on the command line to the testing profile."
  for f in options.extraProfileFiles:
    abspath = getFullPath(f)
    dest = os.path.join(profileDir, os.path.basename(abspath))
    if os.path.isdir(abspath):
      shutil.copytree(abspath, dest)
    else:
      shutil.copy(abspath, dest)

if __name__ == "__main__":
  main()
