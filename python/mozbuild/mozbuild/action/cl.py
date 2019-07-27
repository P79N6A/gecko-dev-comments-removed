



from __future__ import absolute_import

import ctypes
import os
import sys

from mozprocess.processhandler import ProcessHandlerMixin
from mozbuild.makeutil import Makefile

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

    if target is None:
        print >>sys.stderr, "No target set"
        return 1

    
    source = cmdline[-1]
    assert not source.startswith('-')

    
    depstarget = os.path.basename(target) + ".pp"

    cmdline += ['-showIncludes']

    mk = Makefile()
    rule = mk.create_rule([target])
    rule.add_dependencies([normcase(source)])

    def on_line(line):
        
        
        if line.startswith(CL_INCLUDES_PREFIX):
            dep = line[len(CL_INCLUDES_PREFIX):].strip()
            
            
            
            dep = normcase(dep)
            if ' ' not in dep:
                rule.add_dependencies([dep])
        else:
            
            
            sys.stdout.write(line)
            sys.stdout.write('\n')

    
    
    
    p = ProcessHandlerMixin(cmdline, processOutputLine=[on_line],
        ignore_children=True)
    p.run()
    p.processOutput()
    ret = p.wait()

    if ret != 0 or target == "":
        
        
        return int(ret)

    depsdir = os.path.normpath(os.path.join(os.curdir, ".deps"))
    depstarget = os.path.join(depsdir, depstarget)
    if not os.path.isdir(depsdir):
        try:
            os.makedirs(depsdir)
        except OSError:
            pass 
                 
                 

    with open(depstarget, "w") as f:
        mk.dump(f)

    return 0

def main(args):
    return InvokeClWithDependencyGeneration(args)

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
