



import os

from mozboot.base import BaseBootstrapper

class UbuntuBootstrapper(BaseBootstrapper):
    def __init__(self, version, dist_id):
        BaseBootstrapper.__init__(self)

        self.version = version
        self.dist_id = dist_id

    def install_system_packages(self):
        self.run_as_root(['apt-get', 'build-dep', 'firefox'])

        self.apt_install(
            'autoconf2.13',
            'libasound2-dev',
            'libcurl4-openssl-dev',
            'libiw-dev',
            'libnotify-dev',
            'libxt-dev',
            'mercurial',
            'mesa-common-dev',
            'uuid',
            'yasm')
