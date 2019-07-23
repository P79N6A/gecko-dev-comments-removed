




































"""A set of functions to set up a Firefox browser with the correct
   preferences and extensions in the given directory.

"""

__author__ = 'annie.sullivan@gmail.com (Annie Sullivan)'


import platform
import os
import re
import shutil
import tempfile
import time
import glob

import utils
import ffprocess

if platform.system() == "Linux":
    from ffprofile_unix import *
elif platform.system() in ("Windows", "Microsoft"):
    from ffprofile_win32 import *
elif platform.system() == "Darwin":
    from ffprofile_unix import *

def PrefString(name, value, newline):
  """Helper function to create a pref string for Firefox profile prefs.js
     in the form 'user_pref("name", value);<newline>'

  Args:
    name: String containing name of pref
    value: String containing value of pref
    newline: Line ending to use, i.e. '\n' or '\r\n'

  Returns:
    String containing 'user_pref("name", value);<newline>'
  """

  out_value = str(value)
  if type(value) == bool:
    
    out_value = out_value.lower()
  if type(value) == str:
    
    out_value = '"%s"' % value
  return 'user_pref("%s", %s);%s' % (name, out_value, newline)


def CreateTempProfileDir(source_profile, prefs, extensions):
  """Creates a temporary profile directory from the source profile directory
     and adds the given prefs and links to extensions.

  Args:
    source_profile: String containing the absolute path of the source profile
                    directory to copy from.
    prefs: Preferences to set in the prefs.js file of the new profile.  Format:
           {"PrefName1" : "PrefValue1", "PrefName2" : "PrefValue2"}
    extensions: Guids and paths of extensions to link to.  Format:
                {"{GUID1}" : "c:\\Path\\to\\ext1", "{GUID2}", "c:\\Path\\to\\ext2"}

  Returns:
    String containing the absolute path of the profile directory.
  """

  
  
  temp_dir = tempfile.mkdtemp()
  profile_dir = os.path.join(temp_dir, 'profile')
  shutil.copytree(source_profile, profile_dir)
  MakeDirectoryContentsWritable(profile_dir)

  
  user_js_filename = os.path.join(profile_dir, 'user.js')
  user_js_file = open(user_js_filename, 'w')
  for pref in prefs:
    user_js_file.write(PrefString(pref, prefs[pref], '\n'))
  user_js_file.close()

  
  extension_dir = os.path.join(profile_dir, "extensions")
  if not os.path.exists(extension_dir):
    os.makedirs(extension_dir)
  for extension in extensions:
    link_file = open(os.path.join(extension_dir, extension), 'w')
    link_file.write(extensions[extension])
    link_file.close()

  return temp_dir, profile_dir

def InstallInBrowser(firefox_path, dir_path):
  """
    Take the given directory and copies it to appropriate location in the given
    firefox install
  """
  
  fromfiles = glob.glob(os.path.join(dir_path, '*'))
  todir = os.path.join(os.path.dirname(firefox_path), os.path.basename(os.path.normpath(dir_path)))
  for fromfile in fromfiles:
      if not os.path.isfile(os.path.join(todir, os.path.basename(fromfile))):
          shutil.copy(fromfile, todir)
          utils.debug("installed " + fromfile)
      else:
          utils.debug("WARNING: file already installed (" + fromfile + ")")

def InitializeNewProfile(firefox_path, profile_dir, init_url):
  """Runs Firefox with the new profile directory, to negate any performance
     hit that could occur as a result of starting up with a new profile.  
     Also kills the "extra" Firefox that gets spawned the first time Firefox
     is run with a new profile.

  Args:
    firefox_path: String containing the path to the Firefox exe
    profile_dir: The full path to the profile directory to load
  """
  PROFILE_REGEX = re.compile('__metrics(.*)__metrics', re.DOTALL|re.MULTILINE)
  res = 1
  cmd = ffprocess.GenerateFirefoxCommandLine(firefox_path, profile_dir, init_url)
  (match, timed_out) = ffprocess.RunProcessAndWaitForOutput(cmd,
                                                              'firefox',
                                                              PROFILE_REGEX,
                                                              30)
  if (not timed_out):
    print match
  else:
    res = 0
  return res
