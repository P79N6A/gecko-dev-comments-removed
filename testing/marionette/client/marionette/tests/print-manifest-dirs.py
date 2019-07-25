



from manifestparser import TestManifest
import os.path
import sys

def print_test_dirs(topsrcdir, manifest_file):
    """
    Simple routine which prints the paths of directories specified
    in a Marionette manifest, relative to topsrcdir.  This does not recurse 
    into manifests, as we currently have no need for that.
    """

    dirs = set()
    
    topsrcdir = os.path.abspath(topsrcdir)
    scriptdir = os.path.abspath(os.path.dirname(__file__))
    dirs.add(scriptdir[len(topsrcdir) + 1:])

    
    manifest = TestManifest()
    manifest.read(manifest_file)
    for i in manifest.get():
        d = os.path.dirname(i['manifest'])[len(topsrcdir) + 1:]
        dirs.add(d)
    for path in dirs:
        path = path.replace('\\', '/')
        print path

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print >>sys.stderr, "Usage: %s topsrcdir manifest.ini" % sys.argv[0]
        sys.exit(1)

    print_test_dirs(sys.argv[1], sys.argv[2])

