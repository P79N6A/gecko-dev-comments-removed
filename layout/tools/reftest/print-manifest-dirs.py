




import os
import sys
from reftest import ReftestManifest

def printTestDirs(topsrcdir, topmanifests):
    """Parse |topmanifests| and print a list of directories containing the tests
    within (and the manifests including those tests), relative to |topsrcdir|.
    """
    topsrcdir = os.path.abspath(topsrcdir)
    dirs = set()
    for path in topmanifests:
        m = ReftestManifest()
        m.load(path)
        dirs |= m.dirs

    for d in sorted(dirs):
        d = d[len(topsrcdir):].replace('\\', '/').lstrip('/')
        print(d)

if __name__ == '__main__':
    if len(sys.argv) < 3:
      print >>sys.stderr, "Usage: %s topsrcdir reftest.list [reftest.list]*" % sys.argv[0]
      sys.exit(1)
    printTestDirs(sys.argv[1], sys.argv[2:])
