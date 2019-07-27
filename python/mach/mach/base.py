



from __future__ import absolute_import, unicode_literals


class CommandContext(object):
    """Holds run-time state so it can easily be passed to command providers."""
    def __init__(self, cwd=None, settings=None, log_manager=None,
        commands=None, **kwargs):
        self.cwd = cwd
        self.settings = settings
        self.log_manager = log_manager
        self.commands = commands

        for k,v in kwargs.items():
            setattr(self, k, v)


class MachError(Exception):
    """Base class for all errors raised by mach itself."""


class NoCommandError(MachError):
    """No command was passed into mach."""


class UnknownCommandError(MachError):
    """Raised when we attempted to execute an unknown command."""

    def __init__(self, command, verb, suggested_commands=None):
        MachError.__init__(self)

        self.command = command
        self.verb = verb
        self.suggested_commands = suggested_commands or []

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

        
        'docstring',

        
        
        'conditions',

        
        
        '_parser',
        'parser',

        
        
        'arguments',

        
        'argument_group_names',

        
        
        'subcommand_handlers',
    )

    def __init__(self, cls, method, command, pass_context=False):
        self.cls = cls
        self.method = method
        self.name = command.subcommand if command.subcommand else command.name
        self.category = command.category
        self.description = command.description
        self.conditions = command.conditions
        self.arguments = command.arguments
        self.argument_group_names = command.argument_group_names
        self._parser = command.parser
        self.docstring = cls.__dict__[method].__doc__
        self.pass_context = pass_context
        self.subcommand_handlers = {}

    @property
    def parser(self):
        
        
        if callable(self._parser):
            self._parser = self._parser()
        return self._parser

