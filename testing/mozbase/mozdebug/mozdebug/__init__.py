



"""
This module contains a set of function to gather information about the
debugging capabilities of the platform. It allows to look for a specific
debugger or to query the system for a compatible/default debugger.

The following simple example looks for the default debugger on the
current platform and launches a debugger process with the correct
debugger-specific arguments:

::

  import mozdebug

  debugger = mozdebug.get_default_debugger_name()
  debuggerInfo = mozdebug.get_debugger_info(debugger)

  debuggeePath = "toDebug"

  processArgs = [self.debuggerInfo.path] + self.debuggerInfo.args
  processArgs.append(debuggeePath)

  run_process(args, ...)

"""

from mozdebug import *
