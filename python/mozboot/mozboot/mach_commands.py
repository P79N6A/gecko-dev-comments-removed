



from __future__ import absolute_import, unicode_literals

from mach.decorators import (
    CommandProvider,
    Command,
)


@CommandProvider
class Bootstrap(object):
    """Bootstrap system and mach for optimal development experience."""

    @Command('bootstrap', category='devenv',
             description='Install required system packages for building.')
    def bootstrap(self):
        from mozboot.bootstrap import Bootstrapper

        bootstrapper = Bootstrapper()
        bootstrapper.bootstrap()
