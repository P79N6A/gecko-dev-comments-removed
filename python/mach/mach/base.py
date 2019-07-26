



from __future__ import unicode_literals


class CommandContext(object):
    """Holds run-time state so it can easily be passed to command providers."""
    def __init__(self, cwd=None, settings=None, log_manager=None,
        commands=None):
        self.cwd = cwd
        self.settings = settings
        self.log_manager = log_manager
        self.commands = commands


class MachError(Exception):
    """Base class for all errors raised by mach itself."""


class NoCommandError(MachError):
    """No command was passed into mach."""


class UnknownCommandError(MachError):
    """Raised when we attempted to execute an unknown command."""

    def __init__(self, command, verb):
        MachError.__init__(self)

        self.command = command
        self.verb = verb

class UnrecognizedArgumentError(MachError):
    """Raised when an unknown argument is passed to mach."""

    def __init__(self, command, arguments):
        MachError.__init__(self)

        self.command = command
        self.arguments = arguments


class MethodHandler(object):
    """Describes a Python method that implements a mach command.

    Instances of these are produced by mach when it processes classes
    defining mach commands.
    """
    __slots__ = (
        
        
        
        'cls',

        
        
        
        'pass_context',

        
        
        
        'method',

        
        'name',

        
        'category',

        
        'description',

        
        'allow_all_arguments',

        
        
        'conditions',

        
        
        'arguments',
    )

    def __init__(self, cls, method, name, category=None, description=None,
        allow_all_arguments=False, conditions=None, arguments=None,
        pass_context=False):

        self.cls = cls
        self.method = method
        self.name = name
        self.category = category
        self.description = description
        self.allow_all_arguments = allow_all_arguments
        self.conditions = conditions or []
        self.arguments = arguments or []
        self.pass_context = pass_context

