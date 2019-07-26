




"""Extracts a single file from a CAB archive."""

import os
import shutil
import subprocess
import sys
import tempfile

def run_quiet(*args):
  """Run 'expand' supressing noisy output. Returns returncode from process."""
  popen = subprocess.Popen(args, stdout=subprocess.PIPE)
  out, _ = popen.communicate()
  if popen.returncode:
    
    print out
  return popen.returncode

def main():
  if len(sys.argv) != 4:
    print 'Usage: extract_from_cab.py cab_path archived_file output_dir'
    return 1

  [cab_path, archived_file, output_dir] = sys.argv[1:]

  
  
  
  
  temp_dir = tempfile.mkdtemp(dir=output_dir)

  try:
    
    level = run_quiet('expand', cab_path, '-F:' + archived_file, temp_dir)
    if level == 0:
      
      
      output_file = os.path.join(output_dir, archived_file)
      try:
        os.remove(output_file)
      except OSError:
        pass
      os.rename(os.path.join(temp_dir, archived_file), output_file)
  finally:
    shutil.rmtree(temp_dir, True)

  if level != 0:
    return level

  
  
  
  
  os.utime(os.path.join(output_dir, archived_file), None)
  return 0


if __name__ == '__main__':
  sys.exit(main())
