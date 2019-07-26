



import os

from mozboot.base import BaseBootstrapper

class OpenBSDBootstrapper(BaseBootstrapper):
    def __init__(self, version):
        BaseBootstrapper.__init__(self)

        self.packages = [
            'mercurial',
            'llvm',
            'autoconf-2.13',
            'yasm',
            'gtk+2',
            'dbus-glib',
            'gstreamer-plugins-base',
            'pulseaudio',
            'gmake',
            'gtar',
            'wget',
            'unzip',
            'zip',
        ]

    def install_system_packages(self):
        
        self.run_as_root(['pkg_add', '-z'] + self.packages)
