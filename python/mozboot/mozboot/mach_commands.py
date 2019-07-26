



from __future__ import unicode_literals

from mozbuild.base import MozbuildObject

from mach.base import CommandArgument
from mach.base import CommandProvider
from mach.base import Command

@CommandProvider
class Bootstrap(MozbuildObject):
    """Bootstrap system and mach for optimal development experience."""

    @Command('bootstrap',
        help='Install required system packages for building.')
    def bootstrap(self):
        from mozboot.bootstrap import Bootstrapper

        bootstrapper = Bootstrapper()
        bootstrapper.bootstrap()
