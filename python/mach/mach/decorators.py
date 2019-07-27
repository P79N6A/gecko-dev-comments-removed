



from __future__ import unicode_literals

import argparse
import collections
import inspect
import types

from .base import (
    MachError,
    MethodHandler
)

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
            raise MachError(msg)

        if len(spec.args) == 2:
            pass_context = True

    seen_commands = set()

    
    
    
    
    for attr in sorted(cls.__dict__.keys()):
        value = cls.__dict__[attr]

        if not isinstance(value, types.FunctionType):
            continue

        command_name, category, description, conditions, parser = getattr(
            value, '_mach_command', (None, None, None, None, None))

        if command_name is None:
            continue

        seen_commands.add(command_name)

        if conditions is None and Registrar.require_conditions:
            continue

        msg = 'Mach command \'%s\' implemented incorrectly. ' + \
              'Conditions argument must take a list ' + \
              'of functions. Found %s instead.'

        conditions = conditions or []
        if not isinstance(conditions, collections.Iterable):
            msg = msg % (command_name, type(conditions))
            raise MachError(msg)

        for c in conditions:
            if not hasattr(c, '__call__'):
                msg = msg % (command_name, type(c))
                raise MachError(msg)

        arguments = getattr(value, '_mach_command_args', None)

        argument_group_names = getattr(value, '_mach_command_arg_group_names', None)

        handler = MethodHandler(cls, attr, command_name, category=category,
            description=description, conditions=conditions, parser=parser,
            arguments=arguments, argument_group_names=argument_group_names,
            pass_context=pass_context)

        Registrar.register_command_handler(handler)

    
    
    
    for attr in sorted(cls.__dict__.keys()):
        value = cls.__dict__[attr]

        if not isinstance(value, types.FunctionType):
            continue

        command, subcommand, description = getattr(value, '_mach_subcommand',
            (None, None, None))

        if not command:
            continue

        if command not in seen_commands:
            raise MachError('Command referenced by sub-command does not '
                'exist: %s' % command)

        if command not in Registrar.command_handlers:
            continue

        arguments = getattr(value, '_mach_command_args', None)
        argument_group_names = getattr(value, '_mach_command_arg_group_names', None)

        handler = MethodHandler(cls, attr, subcommand, description=description,
            arguments=arguments, argument_group_names=argument_group_names,
            pass_context=pass_context)
        parent = Registrar.command_handlers[command]

        if parent.parser:
            raise MachError('cannot declare sub commands against a command '
                'that has a parser installed: %s' % command)
        if subcommand in parent.subcommand_handlers:
            raise MachError('sub-command already defined: %s' % subcommand)

        parent.subcommand_handlers[subcommand] = handler

    return cls


class Command(object):
    """Decorator for functions or methods that provide a mach command.

    The decorator accepts arguments that define basic attributes of the
    command. The following arguments are recognized:

         category -- The string category to which this command belongs. Mach's
             help will group commands by category.

         description -- A brief description of what the command does.

         parser -- an optional argparse.ArgumentParser instance or callable
             that returns an argparse.ArgumentParser instance to use as the
             basis for the command arguments.

    For example:

        @Command('foo', category='misc', description='Run the foo action')
        def foo(self):
            pass
    """
    def __init__(self, name, category=None, description=None, conditions=None,
                 parser=None):
        self._name = name
        self._category = category
        self._description = description
        self._conditions = conditions
        self._parser = parser

    def __call__(self, func):
        func._mach_command = (self._name, self._category, self._description,
                              self._conditions, self._parser)

        return func

class SubCommand(object):
    """Decorator for functions or methods that provide a sub-command.

    Mach commands can have sub-commands. e.g. ``mach command foo`` or
    ``mach command bar``. Each sub-command has its own parser and is
    effectively its own mach command.

    The decorator accepts arguments that define basic attributes of the
    sub command:

        command -- The string of the command this sub command should be
        attached to.

        subcommand -- The string name of the sub command to register.

        description -- A textual description for this sub command.
    """
    def __init__(self, command, subcommand, description=None):
        self._command = command
        self._subcommand = subcommand
        self._description = description

    def __call__(self, func):
        func._mach_subcommand = (self._command, self._subcommand,
            self._description)

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
        if kwargs.get('nargs') == argparse.REMAINDER:
            
            
            assert len(args) == 1
            assert all(k in ('default', 'nargs', 'help', 'group') for k in kwargs)
        self._command_args = (args, kwargs)

    def __call__(self, func):
        command_args = getattr(func, '_mach_command_args', [])

        command_args.insert(0, self._command_args)

        func._mach_command_args = command_args

        return func


class CommandArgumentGroup(object):
    """Decorator for additional argument groups to mach commands.

    This decorator should be used to add arguments groups to mach commands.
    Arguments to the decorator are proxied to
    ArgumentParser.add_argument_group().

    For example:

        @Command('foo', helps='Run the foo action')
        @CommandArgumentGroup('group1')
        @CommandArgument('-b', '--bar', group='group1', action='store_true',
            default=False, help='Enable bar mode.')
        def foo(self):
            pass

    The name should be chosen so that it makes sense as part of the phrase
    'Command Arguments for <name>' because that's how it will be shown in the
    help message.
    """
    def __init__(self, group_name):
        self._group_name = group_name

    def __call__(self, func):
        command_arg_group_names = getattr(func, '_mach_command_arg_group_names', [])

        command_arg_group_names.insert(0, self._group_name)

        func._mach_command_arg_group_names = command_arg_group_names

        return func


def SettingsProvider(cls):
    """Class decorator to denote that this class provides Mach settings.

    When this decorator is encountered, the underlying class will automatically
    be registered with the Mach registrar and will (likely) be hooked up to the
    mach driver.

    This decorator is only allowed on mach.config.ConfigProvider classes.
    """
    if not issubclass(cls, ConfigProvider):
        raise MachError('@SettingsProvider encountered on class that does ' +
                        'not derived from mach.config.ConfigProvider.')

    Registrar.register_settings_provider(cls)

    return cls

