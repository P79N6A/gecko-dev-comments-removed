



import os

from mozboot.base import BaseBootstrapper

class MintBootstrapper(BaseBootstrapper):
    def __init__(self, version, dist_id):
        BaseBootstrapper.__init__(self)

        self.version = version
        self.dist_id = dist_id

    def install_system_packages(self):
        os.system("sudo apt-get build-dep firefox")
        os.system("sudo apt-get install mercurial libasound2-dev libcurl4-openssl-dev libnotify-dev libxt-dev libiw-dev mesa-common-dev autoconf2.13 yasm uuid")
