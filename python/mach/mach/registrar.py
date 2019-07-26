



from __future__ import unicode_literals

from .base import MachError


class MachRegistrar(object):
    """Container for mach command and config providers."""

    def __init__(self):
        self.command_handlers = {}
        self.commands_by_category = {}
        self.settings_providers = set()
        self.categories = {}
        self.require_conditions = False

    def register_command_handler(self, handler):
        name = handler.name

        if not handler.category:
            raise MachError('Cannot register a mach command without a '
                'category: %s' % name)

        if handler.category not in self.categories:
            raise MachError('Cannot register a command to an undefined '
                'category: %s -> %s' % (name, handler.category))

        self.command_handlers[name] = handler
        self.commands_by_category[handler.category].add(name)

    def register_settings_provider(self, cls):
        self.settings_providers.add(cls)

    def register_category(self, name, title, description, priority=50):
        self.categories[name] = (title, description, priority)
        self.commands_by_category[name] = set()

    def dispatch(self, name, context=None, **args):
        """Dispatch/run a command.

        Commands can use this to call other commands.
        """

        
        
        handler = self.command_handlers[name]
        cls = handler.cls

        if handler.pass_context and not context:
            raise Exception('mach command class requires context.')

        if handler.pass_context:
            instance = cls(context)
        else:
            instance = cls()

        fn = getattr(instance, handler.method)

        return fn(**args) or 0


Registrar = MachRegistrar()
