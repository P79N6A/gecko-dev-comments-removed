



from __future__ import unicode_literals

import time

from mozbuild.base import MozbuildObject

from mach.base import CommandArgument
from mach.base import CommandProvider
from mach.base import Command

import mach.test.common2 as common2


@CommandProvider
class TestCommandProvider(MozbuildObject):
    @Command('throw')
    @CommandArgument('--message', '-m', default='General Error')
    def throw(self, message):
        raise Exception(message)

    @Command('throw_deep')
    @CommandArgument('--message', '-m', default='General Error')
    def throw_deep(self, message):
        common2.throw_deep(message)

