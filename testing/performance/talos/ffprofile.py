



































"""A set of functions to set up a Firefox profile with the correct
   preferences and extensions in the given directory.

   Sets up the profile by copying from a base directory, editing the
   prefs.js file to set the prefs, and creating a file to link to each
   extension.  The profile is run with Firefox to make sure it is fully
   initialized and won't cause extra startup time on the first run.
"""

__author__ = 'annie.sullivan@gmail.com (Annie Sullivan)'


import os
import re
import shutil
import tempfile
import time

import ffprocess
import config

if config.OS is "linux":
    from ffprofile_linux import *
elif config.OS is "win32":
    from ffprofile_win32 import *


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

  
  
  profile_dir = tempfile.mkdtemp()
  profile_dir = os.path.join(profile_dir, 'profile')
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

  return profile_dir


def InitializeNewProfile(firefox_path, profile_dir):
  """Runs Firefox with the new profile directory, to negate any performance
     hit that could occur as a result of starting up with a new profile.  
     Also kills the "extra" Firefox that gets spawned the first time Firefox
     is run with a new profile.

  Args:
    firefox_path: String containing the path to the Firefox exe
    profile_dir: The full path to the profile directory to load
  """

  
  cmd = ffprocess.GenerateFirefoxCommandLine(firefox_path, profile_dir, config.INIT_URL)
  handle = os.popen(cmd)

  
  
  time_elapsed = 0
  while time_elapsed < 30:
    time_elapsed += 5
    time.sleep(5)
    if not ffprocess.ProcessesWithNameExist("firefox"):
      return

  ffprocess.TerminateAllProcesses("firefox")
