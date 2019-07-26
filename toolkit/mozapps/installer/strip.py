



'''
Strip all files that can be stripped in the given directory.
'''

import sys
from mozpack.files import FileFinder
from mozpack.copier import FileCopier

def strip(dir):
    copier = FileCopier()
    
    
    
    for p, f in FileFinder(dir):
        copier.add(p, f)
    copier.copy(dir)

if __name__ == '__main__':
    strip(sys.argv[1])
