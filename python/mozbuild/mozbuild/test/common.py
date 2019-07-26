



from __future__ import unicode_literals

import os

from mach.logging import LoggingManager



log_manager = LoggingManager()
log_manager.add_terminal_logging()



class MockConfig(object):
    def __init__(self, topsrcdir='/path/to/topsrcdir'):
        self.topsrcdir = topsrcdir
        self.topobjdir = '/path/to/topobjdir'

        self.substs = {
            'MOZ_FOO': 'foo',
            'MOZ_BAR': 'bar',
            'MOZ_TRUE': '1',
            'MOZ_FALSE': '',
        }

    def child_path(self, p):
        return os.path.join(self.topsrcdir, p)
