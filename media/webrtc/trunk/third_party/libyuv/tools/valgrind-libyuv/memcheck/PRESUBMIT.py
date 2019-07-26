








"""
Copied from Chrome's src/tools/valgrind/memcheck/PRESUBMIT.py

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details on the presubmit API built into gcl.
"""

import os
import re
import sys

def CheckChange(input_api, output_api):
  """Checks the memcheck suppressions files for bad data."""

  
  tools_vg_path = os.path.join(input_api.PresubmitLocalPath(), '..', '..',
                               'valgrind')
  sys.path.append(tools_vg_path)
  import suppressions

  sup_regex = re.compile('suppressions.*\.txt$')
  suppressions = {}
  errors = []
  check_for_memcheck = False
  
  
  
  
  skip_next_line = False
  for f in filter(lambda x: sup_regex.search(x.LocalPath()),
                  input_api.AffectedFiles()):
    for line, line_num in zip(f.NewContents(),
                              xrange(1, len(f.NewContents()) + 1)):
      line = line.lstrip()
      if line.startswith('#') or not line:
        continue

      if skip_next_line:
        if skip_next_line == 'skip_suppression_name':
          if 'insert_a_suppression_name_here' in line:
            errors.append('"insert_a_suppression_name_here" is not a valid '
                          'suppression name')
          if suppressions.has_key(line):
            if f.LocalPath() == suppressions[line][1]:
              errors.append('suppression with name "%s" at %s line %s '
                            'has already been defined at line %s' %
                            (line, f.LocalPath(), line_num,
                             suppressions[line][1]))
            else:
              errors.append('suppression with name "%s" at %s line %s '
                            'has already been defined at %s line %s' %
                            (line, f.LocalPath(), line_num,
                             suppressions[line][0], suppressions[line][1]))
          else:
            suppressions[line] = (f, line_num)
            check_for_memcheck = True;
        skip_next_line = False
        continue
      if check_for_memcheck:
        if not line.startswith('Memcheck:'):
          errors.append('"%s" should be "Memcheck:..." in %s line %s' %
                        (line, f.LocalPath(), line_num))
        check_for_memcheck = False;
      if line == '{':
        skip_next_line = 'skip_suppression_name'
        continue
      if line == "Memcheck:Param":
        skip_next_line = 'skip_param'
        continue

      if (line.startswith('fun:') or line.startswith('obj:') or
          line.startswith('Memcheck:') or line == '}' or
          line == '...'):
        continue
      errors.append('"%s" is probably wrong: %s line %s' % (line, f.LocalPath(),
                                                            line_num))
  if errors:
    return [output_api.PresubmitError('\n'.join(errors))]
  return []

def CheckChangeOnUpload(input_api, output_api):
  return CheckChange(input_api, output_api)

def CheckChangeOnCommit(input_api, output_api):
  return CheckChange(input_api, output_api)

def GetPreferredTrySlaves():
  
  
  return []
