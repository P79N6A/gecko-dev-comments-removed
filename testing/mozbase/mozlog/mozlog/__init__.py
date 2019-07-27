



"""
Mozlog aims to standardize log handling and formatting within Mozilla.

It implements a JSON-based structured logging protocol with convenience
facilities for recording test results.

The old unstructured module is deprecated. It simply wraps Python's
logging_ module and adds a few convenience methods for logging test
results and events.
"""

import sys

from . import commandline
from . import structuredlog
from . import unstructured
from .structuredlog import get_default_logger, set_default_logger


structured = sys.modules[__name__]
sys.modules['{}.structured'.format(__name__)] = structured
