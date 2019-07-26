










from __future__ import print_function

import sys
import re

def get_prerelease_suffix(version):
  """ Returns the prerelease suffix from the version string argument """

  def mfunc(m):
    return " {0} {1} {2}".format(m.group('prefix'),
                                 {'a': 'Alpha', 'b': 'Beta'}[m.group('c')],
                                 m.group('suffix'))
  result, c = re.subn(r'^(?P<prefix>(\d+\.)*\d+)(?P<c>[ab])(?P<suffix>\d+)$',
                      mfunc, version)
  if c != 1:
    return ''
  return result

if len(sys.argv) == 2:
  print(get_prerelease_suffix(sys.argv[1]))
