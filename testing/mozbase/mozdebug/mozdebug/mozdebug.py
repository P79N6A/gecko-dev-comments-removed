





import os
import mozinfo
from collections import namedtuple
from distutils.spawn import find_executable

__all__ = ['get_debugger_info',
           'get_default_debugger_name',
           'DebuggerSearch']

'''
Map of debugging programs to information about them, like default arguments
and whether or not they are interactive.

To add support for a new debugger, simply add the relative entry in
_DEBUGGER_INFO and optionally update the _DEBUGGER_PRIORITIES.
'''
_DEBUGGER_INFO = {
    
    
    'gdb': {
        'interactive': True,
        'args': ['-q', '--args']
    },

    'cgdb': {
        'interactive': True,
        'args': ['-q', '--args']
    },

    'lldb': {
        'interactive': True,
        'args': ['--'],
        'requiresEscapedArgs': True
    },

    
    'devenv.exe': {
        'interactive': True,
        'args': ['-debugexe']
    },

    
    'wdexpress.exe': {
        'interactive': True,
        'args': ['-debugexe']
    },

    
    
    
    
    
    
    'valgrind': {
        'interactive': False,
        'args': ['--leak-check=full',
                '--show-possibly-lost=no',
                '--smc-check=all-non-file',
                '--vex-iropt-register-updates=allregs-at-mem-access']
    }
}


_DEBUGGER_PRIORITIES = {
      'win': ['devenv.exe', 'wdexpress.exe'],
      'linux': ['gdb', 'cgdb', 'lldb'],
      'mac': ['lldb', 'gdb'],
      'unknown': ['gdb']
}

def get_debugger_info(debugger, debuggerArgs = None, debuggerInteractive = False):
    '''
    Get the information about the requested debugger.

    Returns a dictionary containing the |path| of the debugger executable,
    if it will run in |interactive| mode, its arguments and whether it needs
    to escape arguments it passes to the debugged program (|requiresEscapedArgs|).
    If the debugger cannot be found in the system, returns |None|.

    :param debugger: The name of the debugger.
    :param debuggerArgs: If specified, it's the list of arguments to pass to the
     debugger. A debugger specific separator arguments is appended at the end of
     that list.
    :param debuggerInteractive: If specified, forces the debugger to be interactive.
    '''

    debuggerPath = None

    if debugger:
        
        
        if (os.name == 'nt'
            and not debugger.lower().endswith('.exe')):
            debugger += '.exe'

        debuggerPath = find_executable(debugger)

    if not debuggerPath:
        print 'Error: Could not find debugger %s.' % debugger
        return None

    debuggerName = os.path.basename(debuggerPath).lower()

    def get_debugger_info(type, default):
        if debuggerName in _DEBUGGER_INFO and type in _DEBUGGER_INFO[debuggerName]:
            return _DEBUGGER_INFO[debuggerName][type]
        return default

    
    DebuggerInfo = namedtuple(
        'DebuggerInfo',
        ['path', 'interactive', 'args', 'requiresEscapedArgs']
    )

    debugger_arguments = get_debugger_info('args', [])

    
    if debuggerArgs:
        
        debugger_arguments += [debuggerArgs];

    
    debugger_interactive = get_debugger_info('interactive', False)
    if debuggerInteractive:
        debugger_interactive = debuggerInteractive

    d = DebuggerInfo(
        debuggerPath,
        debugger_interactive,
        debugger_arguments,
        get_debugger_info('requiresEscapedArgs', False)
    )

    return d


class DebuggerSearch:
  OnlyFirst = 1
  KeepLooking = 2

def get_default_debugger_name(search=DebuggerSearch.OnlyFirst):
    '''
    Get the debugger name for the default debugger on current platform.

    :param search: If specified, stops looking for the debugger if the
     default one is not found (|DebuggerSearch.OnlyFirst|) or keeps
     looking for other compatible debuggers (|DebuggerSearch.KeepLooking|).
    '''

    
    debuggerPriorities = _DEBUGGER_PRIORITIES[mozinfo.os if mozinfo.os in _DEBUGGER_PRIORITIES else 'unknown']

    
    for debuggerName in debuggerPriorities:
        debuggerPath = find_executable(debuggerName)
        if debuggerPath:
            return debuggerName
        elif not search == DebuggerSearch.KeepLooking:
            return None

    return None
