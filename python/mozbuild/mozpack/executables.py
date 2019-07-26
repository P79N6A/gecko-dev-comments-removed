



import os
import struct
from buildconfig import (
    substs,
    topobjdir,
)
import subprocess
from mozpack.errors import errors

MACHO_SIGNATURES = [
    0xfeedface,  
    0xcefaedfe,  
    0xfeedfacf,  
    0xcffaedfe,  
]

FAT_SIGNATURE = 0xcafebabe  

ELF_SIGNATURE = 0x7f454c46  

UNKNOWN = 0
MACHO = 1
ELF = 2

def get_type(path):
    '''
    Check the signature of the give file and returns what kind of executable
    matches.
    '''
    with open(path, 'rb') as f:
        signature = f.read(4)
        if len(signature) < 4:
            return UNKNOWN
        signature = struct.unpack('>L', signature)[0]
        if signature == ELF_SIGNATURE:
            return ELF
        if signature in MACHO_SIGNATURES:
            return MACHO
        if signature != FAT_SIGNATURE:
            return UNKNOWN
        
        
        
        
        
        
        
        num = f.read(4)
        if len(num) < 4:
            return UNKNOWN
        num = struct.unpack('>L', num)[0]
        if num < 20:
            return MACHO
        return UNKNOWN


def is_executable(path):
    '''
    Return whether a given file path points to an executable or a library,
    where an executable or library is identified by:
        - the file extension on OS/2
        - the file signature on OS/X and ELF systems (GNU/Linux, Android, BSD,
          Solaris)

    As this function is intended for use to choose between the ExecutableFile
    and File classes in FileFinder, and choosing ExecutableFile only matters
    on OS/2, OS/X and ELF systems, we don't bother detecting other kind of
    executables.
    '''
    if not os.path.exists(path):
        return False

    if substs['OS_ARCH'] == 'OS2':
        return path.lower().endswith((substs['DLL_SUFFIX'],
                                      substs['BIN_SUFFIX']))

    return get_type(path) != UNKNOWN


def may_strip(path):
    '''
    Return whether strip() should be called
    '''
    return not substs['PKG_SKIP_STRIP']


def strip(path):
    '''
    Execute the STRIP command with STRIP_FLAGS on the given path.
    '''
    strip = substs['STRIP']
    flags = substs['STRIP_FLAGS'].split() if 'STRIP_FLAGS' in substs else []
    cmd = [strip] + flags + [path]
    if subprocess.call(cmd) != 0:
        errors.fatal('Error executing ' + ' '.join(cmd))


def may_elfhack(path):
    '''
    Return whether elfhack() should be called
    '''
    
    
    return 'USE_ELF_HACK' in substs and substs['USE_ELF_HACK'] and \
           path.endswith(substs['DLL_SUFFIX'])


def elfhack(path):
    '''
    Execute the elfhack command on the given path.
    '''
    cmd = [os.path.join(topobjdir, 'build/unix/elfhack/elfhack'), path]
    if 'ELF_HACK_FLAGS' in os.environ:
        cmd[1:0] = os.environ['ELF_HACK_FLAGS'].split()
    if subprocess.call(cmd) != 0:
        errors.fatal('Error executing ' + ' '.join(cmd))
