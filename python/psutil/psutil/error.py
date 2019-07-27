





"""This module is deprecated as exceptions are defined in _error.py
and are supposed to be accessed from 'psutil' namespace as in:
- psutil.NoSuchProcess
- psutil.AccessDenied
- psutil.TimeoutExpired
"""

import warnings
from psutil._error import *

warnings.warn("psutil.error module is deprecated and scheduled for removal; " \
              "use psutil namespace instead", category=DeprecationWarning,
               stacklevel=2)
