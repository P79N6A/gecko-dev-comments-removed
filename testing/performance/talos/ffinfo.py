



































__author__ = 'anodelman@mozilla.com'

import re
import ffprocess
import config

PROFILE_REGEX = re.compile('__metrics(.*)')

def GetMetricsFromBrowser(firefox_path, profile_dir):
  """Opens the browser at the specified URL, prints out the information collected from
     the browser"""
  cmd = ffprocess.GenerateFirefoxCommandLine(firefox_path, profile_dir, config.INFO_URL)
  (match, timed_out) = ffprocess.RunProcessAndWaitForOutput(cmd,
                                                              'firefox',
                                                              PROFILE_REGEX,
                                                              30)
  if (not timed_out):
    print match
  else:
    print "ERROR:no metrics"

