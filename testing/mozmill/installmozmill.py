






































"""
install mozmill and its dependencies
"""

import os
import sys
from subprocess import call

def main(args=None):
  """command line front-end function"""

  args = args or sys.argv[1:]

  
  print 'Python: %s' % sys.version

  
  source=os.path.abspath(os.path.dirname(__file__))

  
  if not len(args):
    destination = source
  elif len(args) == 1:
    destination = os.path.abspath(args[0])
  else:
    print "Usage: %s [destination]" % sys.argv[0]
    sys.exit(1)

  os.chdir(source)

  
  required = ('PACKAGES', 'virtualenv')
  for f in required:
    if not os.path.exists(f):
      print "File not found: " + f
      sys.exit(1)

  
  PACKAGES=file('PACKAGES').read().split()
  assert PACKAGES
  
  
  env = os.environ.copy()
  env.pop('PYTHONHOME', None)
  returncode = call([sys.executable, 'virtualenv/virtualenv.py', destination], env=env)
  if returncode:
    print 'Failure to install virtualenv'
    sys.exit(returncode)
  if sys.platform.startswith('win'):
    pip = os.path.join(destination, 'Scripts', 'pip.exe')
  else:
    pip = os.path.join(destination, 'bin', 'pip')
  returncode = call([pip, 'install'] + PACKAGES, env=env)
  if returncode:
    print 'Failure to install packages'
    sys.exit(returncode)

if __name__ == '__main__':
  main()
