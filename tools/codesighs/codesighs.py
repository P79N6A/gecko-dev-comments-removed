































import sys, os

def sumDirectorySize(path):
    """
    Calculate the total size of all the files under |path|.
    """
    total = 0
    def reraise(e):
        raise e
    for root, dirs, files in os.walk(path, onerror=reraise):
        for f in files:
            total += os.path.getsize(os.path.join(root, f))
    return total

def codesighs(stagepath, installerpath):
    """
    Calculate the total size of the files under |stagepath| on disk
    and print it as "__codesize", and print the size of the file at
    |installerpath| as "__installersize".
    """
    try:
        print "__codesize:%d" % sumDirectorySize(stagepath)
        print "__installersize:%d" % os.path.getsize(installerpath)
    except OSError, e:
        print >>sys.stderr, """Couldn't read file %s.
Perhaps you need to run |make package| or |make installer|?""" % e.filename

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print >>sys.stderr, "Usage: codesighs.py <package stage path> <installer path>"
        sys.exit(1)
    codesighs(sys.argv[1], sys.argv[2])
