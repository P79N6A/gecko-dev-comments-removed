



import sys
import subprocess
import os.path

def main():
  return subprocess.call([sys.executable, "../webrtc/trunk/build/dir_exists.py"] + sys.argv[1:])

if __name__ == '__main__':
  sys.exit(main())
