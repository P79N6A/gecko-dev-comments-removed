



"""
To use mozprofile as an API you can import mozprofile.profile_ and/or the AddonManager_.

``mozprofile.profile`` features a generic ``Profile`` class.  In addition,
subclasses ``FirefoxProfile`` and ``ThundebirdProfile`` are available
with preset preferences for those applications.
"""

from profile import *
from addons import *
from cli import *
from prefs import *
from webapps import *
