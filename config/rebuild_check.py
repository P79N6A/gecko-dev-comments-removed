



import os

def mtime(path):
    return os.stat(path).st_mtime

def rebuild_check(args):
    target = args[0]
    deps = args[1:]
    if not os.path.exists(target):
        print target
        return
    t = mtime(target)

    newer = []
    for dep in deps:
        if mtime(dep) > t:
            newer.append(dep)

    if newer:
        print 'Rebuilding %s because %s changed' % (target, ', '.join(newer))
    else:
        print 'Rebuilding %s for an unknown reason' % target

if __name__ == '__main__':
    import sys
    rebuild_check(sys.argv[1:])
