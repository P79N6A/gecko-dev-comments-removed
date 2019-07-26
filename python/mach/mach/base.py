



from __future__ import unicode_literals

from collections import namedtuple


CommandContext = namedtuple('CommandContext', ['topdir', 'cwd',
    'settings', 'log_manager', 'commands'])


class MethodHandler(object):
    """Describes a Python method that implements a mach command.

    Instances of these are produced by mach when it processes classes
    defining mach commands.
    """
    __slots__ = (
        
        
        
        'cls',

        
        
        
        'pass_context',

        
        
        
        'method',

        
        'parser',

        
        
        'parser_args',

        
        
        'arguments',
    )

    def __init__(self, cls, method, parser_args, arguments=None,
        pass_context=False):

        self.cls = cls
        self.method = method
        self.parser_args = parser_args
        self.arguments = arguments or []
        self.pass_context = pass_context
