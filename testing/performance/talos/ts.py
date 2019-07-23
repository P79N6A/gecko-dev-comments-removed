





































"""A set of functions to run the Ts test.

   The Ts test measures startup time for Firefox.  It does this by running
   Firefox with a special page that takes an argument containing the current
   time, and when the page loads (and Firefox is fully started), it writes
   the difference between that time and the now-current time to stdout.  The
   test is run a few times for different profiles, so we can tell if our
   extension affects startup time.
"""

__author__ = 'annie.sullivan@gmail.com (Annie Sullivan)'


import re
import shutil
import time

import ffprocess
import ffprofile
import paths



TS_REGEX = re.compile('__startuptime,(\d*)')


def IsInteger(number):
  """Helper function to determine if a variable is an integer"""
  try:
    int(number)
  except:
    return False

  return True


def RunStartupTest(firefox_path, profile_dir, num_runs, timeout):
  """Runs the Firefox startup test (Ts) for the given number
     of times and returns the output.  If running with a brand
     new profile, make sure to call InitializeNewProfile() first.

  Args:
    firefox_path: The path to the firefox exe to run
    profile_dir: The directory of the profile to use
    num_runs: The number of times to run the startup test
              (1 extra dummy run at the beginning will be thrown out)
    timeout:  The time in seconds to wait before failing and terminating Firefox

  Returns:
    Array containing startup times in milliseconds
  """

  startup_times = []
  for i in range(-1, num_runs):
    
    ffprocess.SyncAndSleep()

    
    
    time_arg = int(time.time() * 1000)
    url = paths.TS_URL + str(time_arg)
    command_line = ffprocess.GenerateFirefoxCommandLine(firefox_path, profile_dir, url)
    (match, timed_out) = ffprocess.RunProcessAndWaitForOutput(command_line,
                                                              'firefox',
                                                              TS_REGEX,
                                                              timeout)
    if timed_out or not IsInteger(match):
      match = None
    if i > -1 and match and match > 0:
      startup_times.append(match)

  return startup_times


def RunStartupTests(source_profile_dir, profile_configs, num_runs):
  """Runs the startup tests with profiles created from the
     given base profile directory and list of configurations.

  Args:
    source_profile_dir:  Full path to base directory to copy profile from.
    profile_configs:  Array of configuration options for each profile.
      These are of the format:
      [{prefname:prevalue,prefname2:prefvalue2},{extensionguid:path_to_extension}],[{prefname...
    num_runs: Number of times to run startup tests for each profile

  Returns:
    Array of arrays of startup times, one for each profile.
  """

  all_times = []
  for config in profile_configs:
    
    profile_dir = ffprofile.CreateTempProfileDir(source_profile_dir,
                                                 config[0],
                                                 config[1])

    
    
    
    ffprofile.InitializeNewProfile(config[2], profile_dir)

    
    times = RunStartupTest(config[2], profile_dir, 5, 30)
    all_times.append(times)

    
    
    
    ffprocess.SyncAndSleep()
    ffprofile.MakeDirectoryContentsWritable(profile_dir)
    shutil.rmtree(profile_dir)

  return all_times
