




"""Top-level presubmit script for GYP.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into gcl.
"""


PYLINT_BLACKLIST = [
    
    
    'test/lib/TestCmd.py',
    'test/lib/TestCommon.py',
    'test/lib/TestGyp.py',
    
    'pylib/gyp/generator/scons.py',
    'pylib/gyp/generator/xcode.py',
]


PYLINT_DISABLED_WARNINGS = [
    
    
    'W0611',
    
    'F0401',
    
    'W0622',
    
    'W0612',
    
    'C0323',
    'C0322',
    
    'W0301',
    
    'W0613',
    
    'W0105',
    
    'C0324',
    
    'W0212',
    
    'W0311',
    
    'C0301',
    
    'E0602',
    
    'W0702',
    
    'E1101',
    
    'W0102',
    
    'W0201', 'W0232', 'E1103', 'W0621', 'W0108', 'W0223', 'W0231',
    'R0201', 'E0101', 'C0321',
    
    
    'W0104',
]


def CheckChangeOnUpload(input_api, output_api):
  report = []
  report.extend(input_api.canned_checks.PanProjectChecks(
      input_api, output_api))
  return report


def CheckChangeOnCommit(input_api, output_api):
  report = []
  license = (
      r'.*? Copyright \(c\) %(year)s Google Inc\. All rights reserved\.\n'
      r'.*? Use of this source code is governed by a BSD-style license that '
        r'can be\n'
      r'.*? found in the LICENSE file\.\n'
  ) % {
      'year': input_api.time.strftime('%Y'),
  }

  report.extend(input_api.canned_checks.PanProjectChecks(
      input_api, output_api, license_header=license))
  report.extend(input_api.canned_checks.CheckTreeIsOpen(
      input_api, output_api,
      'http://gyp-status.appspot.com/status',
      'http://gyp-status.appspot.com/current'))

  import sys
  old_sys_path = sys.path
  try:
    sys.path = ['pylib', 'test/lib'] + sys.path
    report.extend(input_api.canned_checks.RunPylint(
        input_api,
        output_api,
        black_list=PYLINT_BLACKLIST,
        disabled_warnings=PYLINT_DISABLED_WARNINGS))
  finally:
    sys.path = old_sys_path
  return report


def GetPreferredTrySlaves():
  return ['gyp-win32', 'gyp-win64', 'gyp-linux', 'gyp-mac']
