



from __future__ import unicode_literals

import inspect
import types

from .base import MethodHandler
from .config import ConfigProvider
from .registrar import Registrar


def CommandProvider(cls):
    """Class decorator to denote that it provides subcommands for Mach.

    When this decorator is present, mach looks for commands being defined by
    methods inside the class.
    """

    
    
    
    
    
    

    
    pass_context = False

    if inspect.ismethod(cls.__init__):
        spec = inspect.getargspec(cls.__init__)

        if len(spec.args) > 2:
            msg = 'Mach @CommandProvider class %s implemented incorrectly. ' + \
                  '__init__() must take 1 or 2 arguments. From %s'
            msg = msg % (cls.__name__, inspect.getsourcefile(cls))
            raise Exception(msg)

        if len(spec.args) == 2:
            pass_context = True

    
    
    
    
    for attr in sorted(cls.__dict__.keys()):
        value = cls.__dict__[attr]

        if not isinstance(value, types.FunctionType):
            continue

        parser_args = getattr(value, '_mach_command', None)
        if parser_args is None:
            continue

        arguments = getattr(value, '_mach_command_args', None)

        handler = MethodHandler(cls, attr, (parser_args[0], parser_args[1]),
            arguments=arguments, pass_context=pass_context)

        Registrar.register_command_handler(handler)

    return cls


class Command(object):
    """Decorator for functions or methods that provide a mach subcommand.

    The decorator accepts arguments that would be passed to add_parser() of an
    ArgumentParser instance created via add_subparsers(). Essentially, it
    accepts the arguments one would pass to add_argument().

    For example:

        @Command('foo', help='Run the foo action')
        def foo(self):
            pass
    """
    def __init__(self, *args, **kwargs):
        self._command_args = (args, kwargs)

    def __call__(self, func):
        func._mach_command = self._command_args

        return func


class CommandArgument(object):
    """Decorator for additional arguments to mach subcommands.

    This decorator should be used to add arguments to mach commands. Arguments
    to the decorator are proxied to ArgumentParser.add_argument().

    For example:

        @Command('foo', help='Run the foo action')
        @CommandArgument('-b', '--bar', action='store_true', default=False,
            help='Enable bar mode.')
        def foo(self):
            pass
    """
    def __init__(self, *args, **kwargs):
        self._command_args = (args, kwargs)

    def __call__(self, func):
        command_args = getattr(func, '_mach_command_args', [])

        command_args.insert(0, self._command_args)

        func._mach_command_args = command_args

        return func

def SettingsProvider(cls):
    """Class decorator to denote that this class provides Mach settings.

    When this decorator is encountered, the underlying class will automatically
    be registered with the Mach registrar and will (likely) be hooked up to the
    mach driver.

    This decorator is only allowed on mach.config.ConfigProvider classes.
    """
    if not issubclass(cls, ConfigProvider):
        raise Exception('@SettingsProvider encountered on class that does ' +
                        'not derived from mach.config.ConfigProvider.')

    Registrar.register_settings_provider(cls)

    return cls

