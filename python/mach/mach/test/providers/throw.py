



from __future__ import unicode_literals

import time

from mach.decorators import (
    CommandArgument,
    CommandProvider,
    Command,
)

from mach.test.providers import throw2


@CommandProvider
class TestCommandProvider(object):
    @Command('throw')
    @CommandArgument('--message', '-m', default='General Error')
    def throw(self, message):
        raise Exception(message)

    @Command('throw_deep')
    @CommandArgument('--message', '-m', default='General Error')
    def throw_deep(self, message):
        throw2.throw_deep(message)

