



from __future__ import unicode_literals

from mach.logging import LoggingManager

from mozbuild.util import ReadOnlyDict

import mozpack.path as mozpath



log_manager = LoggingManager()
log_manager.add_terminal_logging()



class MockConfig(object):
    def __init__(self, topsrcdir='/path/to/topsrcdir', extra_substs={}):
        self.topsrcdir = mozpath.abspath(topsrcdir)
        self.topobjdir = mozpath.abspath('/path/to/topobjdir')

        self.substs = ReadOnlyDict({
            'MOZ_FOO': 'foo',
            'MOZ_BAR': 'bar',
            'MOZ_TRUE': '1',
            'MOZ_FALSE': '',
        }, **extra_substs)

        self.substs_unicode = ReadOnlyDict({k.decode('utf-8'): v.decode('utf-8',
            'replace') for k, v in self.substs.items()})

        self.defines = self.substs

        self.external_source_dir = None
        self.lib_prefix = 'lib'
        self.lib_suffix = '.so'
