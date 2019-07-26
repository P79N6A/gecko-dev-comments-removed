



'''Given a library, dependentlibs.py prints the list of libraries it depends
upon that are in the same directory.
'''

from optparse import OptionParser
import os
import re
import fnmatch
import subprocess
import sys
from mozpack.executables import (
    get_type,
    ELF,
    MACHO,
)

TOOLCHAIN_PREFIX = ''

def dependentlibs_dumpbin(lib):
    '''Returns the list of dependencies declared in the given DLL'''
    try:
        proc = subprocess.Popen(['dumpbin', '-dependents', lib], stdout = subprocess.PIPE)
    except OSError:
        
        return dependentlibs_mingw_objdump(lib)
    deps = []
    for line in proc.stdout:
        
        match = re.match('    (\S+)', line)
        if match:
             deps.append(match.group(1))
        elif len(deps):
             
             
             
             break
    proc.wait()
    return deps

def dependentlibs_mingw_objdump(lib):
    proc = subprocess.Popen(['objdump', '-x', lib], stdout = subprocess.PIPE)
    deps = []
    for line in proc.stdout:
        match = re.match('\tDLL Name: (\S+)', line)
        if match:
            deps.append(match.group(1))
    proc.wait()
    return deps

def dependentlibs_readelf(lib):
    '''Returns the list of dependencies declared in the given ELF .so'''
    proc = subprocess.Popen([TOOLCHAIN_PREFIX + 'readelf', '-d', lib], stdout = subprocess.PIPE)
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

def dependentlibs(lib, libpaths, func):
    '''For a given library, returns the list of recursive dependencies that can
    be found in the given list of paths'''
    assert(libpaths)
    assert(isinstance(libpaths, list))
    deps = []
    for dep in func(lib):
        if dep in deps or os.path.isabs(dep):
            continue
        for dir in libpaths:
            deppath = os.path.join(dir, dep)
            if os.path.exists(deppath):
                deps.extend([d for d in dependentlibs(deppath, libpaths, func) if not d in deps])
                deps.append(dep)
                break

    return deps

def main():
    parser = OptionParser()
    parser.add_option("-L", dest="libpaths", action="append", metavar="PATH", help="Add the given path to the library search path")
    parser.add_option("-p", dest="toolchain_prefix", metavar="PREFIX", help="Use the given prefix to readelf")
    (options, args) = parser.parse_args()
    if options.toolchain_prefix:
        global TOOLCHAIN_PREFIX
        TOOLCHAIN_PREFIX = options.toolchain_prefix
    lib = args[0]
    binary_type = get_type(lib)
    if binary_type == ELF:
        func = dependentlibs_readelf
    elif binary_type == MACHO:
        func = dependentlibs_otool
    else:
        ext = os.path.splitext(lib)[1]
        assert(ext == '.dll')
        func = dependentlibs_dumpbin
    if not options.libpaths:
        options.libpaths = [os.path.dirname(lib)]

    print '\n'.join(dependentlibs(lib, options.libpaths, func))

if __name__ == '__main__':
    main()
