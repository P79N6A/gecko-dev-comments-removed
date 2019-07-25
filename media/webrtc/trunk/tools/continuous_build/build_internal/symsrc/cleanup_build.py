








__author__ = 'kjellander@webrtc.org (Henrik Kjellander)'

"""Clean up a build by removing just compile output or everything (nuke).

This script assumes the trunk directory is located directly below the current
working directory.
"""

from optparse import OptionParser
import sys

from common import chromium_utils


def main():
  usage = 'usage: %prog [--nuke]'
  parser = OptionParser(usage)
  parser.add_option('-n', '--nuke',
                    action='store_true', dest='nuke', default=False,
                    help='Nuke whole repository (not just build output)')
  options, unused_args = parser.parse_args()

  if options.nuke:
    chromium_utils.RemoveDirectory('trunk')
  else:
    
    if chromium_utils.IsWindows():
      chromium_utils.RemoveDirectory('trunk\\build\\Debug')
      chromium_utils.RemoveDirectory('trunk\\build\\Release')
    elif chromium_utils.IsMac():
      chromium_utils.RemoveDirectory('trunk/out')
      chromium_utils.RemoveDirectory('trunk/xcodebuild')
    elif chromium_utils.IsLinux():
      chromium_utils.RemoveDirectory('trunk/out')
    else:
      print 'Unknown platform: ' + sys.platform
      return 1
  return 0

if '__main__' == __name__:
  sys.exit(main())
