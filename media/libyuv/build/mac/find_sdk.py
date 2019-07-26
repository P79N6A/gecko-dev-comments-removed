



import sys
import subprocess

def main():
  return subprocess.call([sys.executable, "../webrtc/trunk/build/mac/find_sdk.py"] + sys.argv[1:])

if __name__ == '__main__':
  sys.exit(main())
