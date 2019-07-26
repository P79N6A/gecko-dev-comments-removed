



from __future__ import unicode_literals

import argparse
import sys

from operator import itemgetter

from .base import (
    NoCommandError,
    UnknownCommandError,
    UnrecognizedArgumentError,
)


class CommandAction(argparse.Action):
    """An argparse action that handles mach commands.

    This class is essentially a reimplementation of argparse's sub-parsers
    feature. We first tried to use sub-parsers. However, they were missing
    features like grouping of commands (http://bugs.python.org/issue14037).

    The way this works involves light magic and a partial understanding of how
    argparse works.

    Arguments registered with an argparse.ArgumentParser have an action
    associated with them. An action is essentially a class that when called
    does something with the encountered argument(s). This class is one of those
    action classes.

    An instance of this class is created doing something like:

        parser.add_argument('command', action=CommandAction, registrar=r)

    Note that a mach.registrar.Registrar instance is passed in. The Registrar
    holds information on all the mach commands that have been registered.

    When this argument is registered with the ArgumentParser, an instance of
    this class is instantiated. One of the subtle but important things it does
    is tell the argument parser that it's interested in *all* of the remaining
    program arguments. So, when the ArgumentParser calls this action, we will
    receive the command name plus all of its arguments.

    For more, read the docs in __call__.
    """
    def __init__(self, option_strings, dest, required=True, default=None,
        registrar=None):
        
        
        
        
        
        
        argparse.Action.__init__(self, option_strings, dest, required=required,
            help=argparse.SUPPRESS, nargs=argparse.REMAINDER)

        self._mach_registrar = registrar

    def __call__(self, parser, namespace, values, option_string=None):
        """This is called when the ArgumentParser has reached our arguments.

        Since we always register ourselves with nargs=argparse.REMAINDER,
        values should be a list of remaining arguments to parse. The first
        argument should be the name of the command to invoke and all remaining
        arguments are arguments for that command.

        The gist of the flow is that we look at the command being invoked. If
        it's *help*, we handle that specially (because argparse's default help
        handler isn't satisfactory). Else, we create a new, independent
        ArgumentParser instance for just the invoked command (based on the
        information contained in the command registrar) and feed the arguments
        into that parser. We then merge the results with the main
        ArgumentParser.
        """
        if not values:
            raise NoCommandError()

        command = values[0]
        args = values[1:]

        if command == 'help':
            if len(args):
                self._handle_subcommand_help(parser, args[0])
            else:
                self._handle_main_help(parser)

            sys.exit(0)

        handler = self._mach_registrar.command_handlers.get(command)

        
        
        if not handler:
            raise UnknownCommandError(command, 'run')

        
        
        
        

        
        
        
        

        parser_args = {
            'add_help': False,
            'usage': '%(prog)s [global arguments] ' + command +
                ' command arguments]',
        }

        if handler.allow_all_arguments:
            parser_args['prefix_chars'] = '+'

        subparser = argparse.ArgumentParser(**parser_args)

        for arg in handler.arguments:
            subparser.add_argument(*arg[0], **arg[1])

        
        
        setattr(namespace, 'mach_handler', handler)
        setattr(namespace, 'command', command)

        command_namespace, extra = subparser.parse_known_args(args)
        setattr(namespace, 'command_args', command_namespace)

        if extra:
            raise UnrecognizedArgumentError(command, extra)

    def _handle_main_help(self, parser):
        
        
        
        
        r = self._mach_registrar

        cats = [(k, v[2]) for k, v in r.categories.items()]
        sorted_cats = sorted(cats, key=itemgetter(1), reverse=True)
        for category, priority in sorted_cats:
            title, description, _priority = r.categories[category]

            group = parser.add_argument_group(title, description)

            for command in sorted(r.commands_by_category[category]):
                handler = r.command_handlers[command]
                description = handler.description

                group.add_argument(command, help=description,
                    action='store_true')

        parser.print_help()

    def _handle_subcommand_help(self, parser, command):
        handler = self._mach_registrar.command_handlers.get(command)

        if not handler:
            raise UnknownCommandError(command, 'query')

        group = parser.add_argument_group('Command Arguments')

        for arg in handler.arguments:
            group.add_argument(*arg[0], **arg[1])

        
        description = handler.description
        if description:
            parser.description = description

        parser.usage = '%(prog)s [global arguments] ' + command + \
            ' [command arguments]'
        parser.print_help()

