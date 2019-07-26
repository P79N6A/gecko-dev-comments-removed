



from __future__ import unicode_literals

import multiprocessing

from mach.config import (
    ConfigProvider,
    PositiveIntegerType,
)

from mach.decorators import SettingsProvider


@SettingsProvider
class BuildConfig(ConfigProvider):
    """The configuration for mozbuild."""

    def __init__(self, settings):
        self.settings = settings

    @classmethod
    def _register_settings(cls):
        def register(section, option, type_cls, **kwargs):
            cls.register_setting(section, option, type_cls, domain='mozbuild',
                **kwargs)

        register('build', 'threads', PositiveIntegerType,
            default=multiprocessing.cpu_count())


