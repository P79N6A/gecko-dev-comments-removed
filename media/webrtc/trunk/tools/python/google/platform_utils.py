



"""Platform-specific utilities and pseudo-constants

Any functions whose implementations or values differ from one platform to
another should be defined in their respective platform_utils_<platform>.py
modules. The appropriate one of those will be imported into this module to
provide callers with a common, platform-independent interface.
"""

import sys




if sys.platform in ('cygwin', 'win32'):
  from platform_utils_win import *
elif sys.platform == 'darwin':
  from platform_utils_mac import *
elif sys.platform.startswith('linux'):
  from platform_utils_linux import *
