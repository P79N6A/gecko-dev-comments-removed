



import os
import errno

def mtime(path):
    try:
        return os.stat(path).st_mtime
    except OSError as e:
        if e.errno == errno.ENOENT:
            return -1
        raise

def rebuild_check(args):
    target = args[0]
    deps = args[1:]
    t = mtime(target)
    if t < 0:
        print target
        return

    newer = []
    removed = []
    for dep in deps:
        deptime = mtime(dep)
        if deptime < 0:
            removed.append(dep)
        elif mtime(dep) > t:
            newer.append(dep)

    if newer and removed:
        print 'Rebuilding %s because %s changed and %s was removed' % (target, ', '.join(newer), ', '.join(removed))
    elif newer:
        print 'Rebuilding %s because %s changed' % (target, ', '.join(newer))
    elif removed:
        print 'Rebuilding %s because %s was removed' % (target, ', '.join(removed))
    else:
        print 'Rebuilding %s for an unknown reason' % target

if __name__ == '__main__':
    import sys
    rebuild_check(sys.argv[1:])
