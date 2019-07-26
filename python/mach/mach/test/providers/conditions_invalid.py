



from __future__ import unicode_literals

from mach.decorators import (
    CommandProvider,
    Command,
)

@CommandProvider
class ConditionsProvider(object):
    @Command('cmd_foo', category='testing', conditions=["invalid"])
    def run_foo(self):
        pass
