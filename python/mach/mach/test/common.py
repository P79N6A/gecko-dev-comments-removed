



from __future__ import unicode_literals

import time

from mach.base import (
    CommandArgument,
    CommandProvider,
    Command,
)

import mach.test.common2 as common2


@CommandProvider
class TestCommandProvider(object):
    @Command('throw')
    @CommandArgument('--message', '-m', default='General Error')
    def throw(self, message):
        raise Exception(message)

    @Command('throw_deep')
    @CommandArgument('--message', '-m', default='General Error')
    def throw_deep(self, message):
        common2.throw_deep(message)

