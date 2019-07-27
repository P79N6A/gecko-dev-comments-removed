



"""
Mozlog aims to standardize log formatting within Mozilla.

It simply wraps Python's logging_ module and adds a few convenience methods
for logging test results and events.

The structured submodule takes a different approach and implements a
JSON-based logging protocol designed for recording test results."""

from logger import *
from loglistener import LogMessageServer
from loggingmixin import LoggingMixin

try:
    import structured
except ImportError:
    
    
    
    
    pass

