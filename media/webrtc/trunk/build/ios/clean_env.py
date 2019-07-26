




import os
import sys

def Main(argv):
  """This is like 'env -i', but it uses a whitelist of env variables to allow
  through to the command being run.  It attempts to strip off Xcode-added
  values from PATH.
  """
  
  
  
  env_key_whitelist = (
    'HOME',
    'LOGNAME',
    
    'PWD',
    'SHELL',
    'TEMP',
    'TMPDIR',
    'USER'
  )

  
  
  assert(len(argv) > 0)

  add_to_path = [];
  first_entry = argv[0];
  if first_entry.startswith('ADD_TO_PATH='):
    argv = argv[1:];
    add_to_path = first_entry.replace('ADD_TO_PATH=', '', 1).split(':')

  
  assert(len(argv) > 0)

  clean_env = {}

  
  for key in env_key_whitelist:
    val = os.environ.get(key, None)
    if not val is None:
      clean_env[key] = val

  
  dev_prefix = os.environ.get('DEVELOPER_DIR', '/Developer/')
  if dev_prefix[-1:] != '/':
    dev_prefix += '/'

  
  initial_path = os.environ.get('PATH', '')
  filtered_chunks = \
      [x for x in initial_path.split(':') if not x.startswith(dev_prefix)]
  if filtered_chunks:
    clean_env['PATH'] = ':'.join(add_to_path + filtered_chunks)

  
  args = argv[:]
  while '=' in args[0]:
    (key, val) = args[0].split('=', 1)
    clean_env[key] = val
    args = args[1:]

  
  assert(len(args) > 0)

  
  os.execvpe(args[0], args, clean_env)
  
  return 66

if __name__ == '__main__':
  sys.exit(Main(sys.argv[1:]))
