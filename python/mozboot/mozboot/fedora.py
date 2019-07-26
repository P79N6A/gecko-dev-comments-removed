



import os

from mozboot.base import BaseBootstrapper

class FedoraBootstrapper(BaseBootstrapper):
    def __init__(self, version, dist_id):
        BaseBootstrapper.__init__(self)

        self.version = version
        self.dist_id = dist_id

    def install_system_packages(self):
        self.yum_groupinstall(
            'Development Tools',
            'Development Libraries',
            'GNOME Software Development')

        self.yum_install(
            'alsa-lib-devel',
            'autoconf213',
            'glibc-static',
            'libstdc++-static',
            'libXt-devel',
            'mercurial',
            'mesa-libGL-devel',
            'wireless-tools-devel',
            'yasm')
