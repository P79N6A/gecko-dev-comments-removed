



from __future__ import print_function

import os
import subprocess
import sys

from mozboot.base import BaseBootstrapper

class FreeBSDBootstrapper(BaseBootstrapper):
    def __init__(self, version):
        BaseBootstrapper.__init__(self)
        self.version = int(version.split('.')[0])

    def pkg_install(self, *packages):
        if self.which('pkg'):
            command = ['pkg', 'install', '-x']
            command.extend([i[0] for i in packages])
        else:
            command = ['pkg_add', '-Fr']
            command.extend([i[-1] for i in packages])

        self.run_as_root(command)

    def install_system_packages(self):
        
        if self.version < 9:
            self.pkg_install(('gcc',))

        self.pkg_install(
            ('autoconf-2.13', 'autoconf213'),
            ('dbus-glib',),
            ('gmake',),
            ('gstreamer-plugins',),
            ('gtk-2', 'gtk20'),
            ('libGL',),
            ('libIDL',),
            ('libv4l',),
            ('mercurial',),
            ('pulseaudio',),
            ('yasm',),
            ('zip',))

    def upgrade_mercurial(self, current):
        self.pkg_install('mercurial')
