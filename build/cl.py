



import ctypes
import os, os.path
import subprocess
import sys

CL_INCLUDES_PREFIX = os.environ.get("CL_INCLUDES_PREFIX", "Note: including file:")

GetShortPathName = ctypes.windll.kernel32.GetShortPathNameW
GetLongPathName = ctypes.windll.kernel32.GetLongPathNameW







_normcase_cache = {}

def normcase(path):
    
    path = path.replace('/', os.sep)
    dir = os.path.dirname(path)
    
    
    name = os.path.basename(path)
    if dir in _normcase_cache:
        result = _normcase_cache[dir]
    else:
        path = ctypes.create_unicode_buffer(dir)
        length = GetShortPathName(path, None, 0)
        shortpath = ctypes.create_unicode_buffer(length)
        GetShortPathName(path, shortpath, length)
        length = GetLongPathName(shortpath, None, 0)
        if length > len(path):
            path = ctypes.create_unicode_buffer(length)
        GetLongPathName(shortpath, path, length)
        result = _normcase_cache[dir] = path.value
    return os.path.join(result, name)


def InvokeClWithDependencyGeneration(cmdline):
    target = ""
    
    for arg in cmdline:
        if arg.startswith("-Fo"):
            target = arg[3:]
            break

    if target == None:
        print >>sys.stderr, "No target set" and sys.exit(1)

    
    source = cmdline[-1]
    assert not source.startswith('-')

    
    depstarget = os.path.basename(target) + ".pp"

    cmdline += ['-showIncludes']
    cl = subprocess.Popen(cmdline, stdout=subprocess.PIPE)

    deps = set([normcase(source).replace(os.sep, '/')])
    for line in cl.stdout:
        
        
        if line.startswith(CL_INCLUDES_PREFIX):
            dep = line[len(CL_INCLUDES_PREFIX):].strip()
            
            
            
            if ' ' not in dep:
                deps.add(normcase(dep).replace(os.sep, '/'))
        else:
            sys.stdout.write(line) 
                                   

    ret = cl.wait()
    if ret != 0 or target == "":
        sys.exit(ret)

    depsdir = os.path.normpath(os.path.join(os.curdir, ".deps"))
    depstarget = os.path.join(depsdir, depstarget)
    if not os.path.isdir(depsdir):
        try:
            os.makedirs(depsdir)
        except OSError:
            pass 
                 
                 

    with open(depstarget, "w") as f:
        f.write("%s: %s" % (target, source))
        for dep in sorted(deps):
            f.write(" \\\n%s" % dep)
        f.write('\n')
        for dep in sorted(deps):
            f.write("%s:\n" % dep)

if __name__ == "__main__":
    InvokeClWithDependencyGeneration(sys.argv[1:])
