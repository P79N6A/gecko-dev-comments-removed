



"""Distutils setup script for 'which'."""

import sys
import os
import shutil
from distutils.core import setup




def _getVersion():
    import which
    return which.__version__

def _getBinDir():
    """Return the current Python's bindir."""
    if sys.platform.startswith("win"):
        bindir = sys.prefix
    else:
        bindir = os.path.join(sys.prefix, "bin")
    return bindir




if sys.platform == "win32":
    scripts = []
    binFiles = ["which.exe", "which.py"]
else:
    
    
    
    
    
    
    
    binFiles = []
    scripts = []

setup(name="which",
      version=_getVersion(),
      description="a portable GNU which replacement",
      author="Trent Mick",
      author_email="TrentM@ActiveState.com",
      url="http://trentm.com/projects/which/",
      license="MIT License",
      platforms=["Windows", "Linux", "Mac OS X", "Unix"],
      long_description="""\
This is a GNU which replacement with the following features:
    - it is portable (Windows, Linux);
    - it understands PATHEXT on Windows;
    - it can print <em>all</em> matches on the PATH;
    - it can note "near misses" on the PATH (e.g. files that match but
      may not, say, have execute permissions; and
    - it can be used as a Python module.
""",
      keywords=["which", "find", "path", "where"],

      py_modules=['which'],
      scripts=scripts,
      
      
      
      data_files=[ (_getBinDir(), binFiles) ],
     )

