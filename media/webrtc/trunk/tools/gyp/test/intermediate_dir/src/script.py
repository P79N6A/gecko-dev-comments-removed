








import os
import shlex
import sys

if len(sys.argv) == 3 and ' ' in sys.argv[2]:
  sys.argv[2], fourth = shlex.split(sys.argv[2].replace('\\', '\\\\'))
  sys.argv.append(fourth)



with open(sys.argv[2], 'w') as f:
  f.write(sys.argv[1])

with open(sys.argv[3], 'w') as f:
  f.write(os.path.abspath(sys.argv[2]))
