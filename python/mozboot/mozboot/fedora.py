



import os

from mozboot.base import BaseBootstrapper

class FedoraBootstrapper(BaseBootstrapper):
    def __init__(self, version, dist_id):
        BaseBootstrapper.__init__(self)

        self.version = version
        self.dist_id = dist_id

    def install_system_packages(self):
        os.system("sudo yum groupinstall 'Development Tools' 'Development Libraries' 'GNOME Software Development'")
        os.system("sudo yum install mercurial autoconf213 glibc-static libstdc++-static yasm wireless-tools-devel mesa-libGL-devel alsa-lib-devel libXt-devel")
