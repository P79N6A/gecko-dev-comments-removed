









































"""
utility functions for mozrunner
"""

__all__ = ['findInPath', 'get_metadata_from_egg']

import mozinfo
import os
import sys



try:
    import pkg_resources
    def get_metadata_from_egg(module):
        ret = {}
        dist = pkg_resources.get_distribution(module)
        if dist.has_metadata("PKG-INFO"):
            key = None
            for line in dist.get_metadata("PKG-INFO").splitlines():
                
                if key == 'Description':
                    
                    if not line or line[0].isspace():
                        value += '\n' + line
                        continue
                    else:
                        key = key.strip()
                        value = value.strip()
                        ret[key] = value

                key, value = line.split(':', 1)
                key = key.strip()
                value = value.strip()
                ret[key] = value
        if dist.has_metadata("requires.txt"):
            ret["Dependencies"] = "\n" + dist.get_metadata("requires.txt")
        return ret
except ImportError:
    
    def get_metadata_from_egg(module):
        return {}


def findInPath(fileName, path=os.environ['PATH']):
    """python equivalent of which; should really be in the stdlib"""
    dirs = path.split(os.pathsep)
    for dir in dirs:
        if os.path.isfile(os.path.join(dir, fileName)):
            return os.path.join(dir, fileName)
        if mozinfo.isWin:
            if os.path.isfile(os.path.join(dir, fileName + ".exe")):
                return os.path.join(dir, fileName + ".exe")

if __name__ == '__main__':
    for i in sys.argv[1:]:
        print findInPath(i)
