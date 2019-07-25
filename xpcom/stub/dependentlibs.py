



'''Given a library, dependentlibs.py prints the list of libraries it depends
upon that are in the same directory.
'''

import os
import re
import subprocess
import sys

def dependentlibs_dumpbin(lib):
    '''Returns the list of dependencies declared in the given DLL'''
    proc = subprocess.Popen(['dumpbin', '-imports', lib], stdout = subprocess.PIPE)
    deps = []
    for line in proc.stdout:
        
        match = re.match('    (\S+)', line)
        if match:
             deps.append(match.group(1))
    proc.wait()
    return deps

def dependentlibs_readelf(lib):
    '''Returns the list of dependencies declared in the given ELF .so'''
    proc = subprocess.Popen(['readelf', '-d', lib], stdout = subprocess.PIPE)
    deps = []
    for line in proc.stdout:
        
        
        
        tmp = line.split(' ', 3)
        if len(tmp) > 3 and tmp[2] == '(NEEDED)':
            
            
            match = re.search('\[(.*)\]', tmp[3])
            if match:
                deps.append(match.group(1))
    proc.wait()
    return deps

def dependentlibs_otool(lib):
    '''Returns the list of dependencies declared in the given MACH-O dylib'''
    proc = subprocess.Popen(['otool', '-l', lib], stdout = subprocess.PIPE)
    deps= []
    cmd = None
    for line in proc.stdout:
        
        
        
        
        
        tmp = line.split()
        if len(tmp) < 2:
            continue
        if tmp[0] == 'cmd':
            cmd = tmp[1]
        elif cmd == 'LC_LOAD_DYLIB' and tmp[0] == 'name':
            deps.append(re.sub('^@executable_path/','',tmp[1]))
    proc.wait()
    return deps

def dependentlibs(lib, func):
    '''For a given library, returns the list of recursive dependencies that are
    in the same directory'''
    deps = []
    dir = os.path.dirname(lib)
    for dep in func(lib):
        if dep in deps or os.path.isabs(dep):
            continue
        deppath = os.path.join(dir, dep)
        if os.path.exists(deppath):
            deps.extend([d for d in dependentlibs(deppath, func) if not d in deps])
            deps.append(dep)

    return deps

def main():
    lib = sys.argv[1]
    ext = os.path.splitext(lib)[1]
    if ext == '.dll':
        func = dependentlibs_dumpbin
    elif ext == '.so':
        func = dependentlibs_readelf
    elif ext == '.dylib':
        func = dependentlibs_otool

    print '\n'.join(dependentlibs(lib, func))

if __name__ == '__main__':
    main()
