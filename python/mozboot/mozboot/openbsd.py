



import os

from mozboot.base import BaseBootstrapper

class OpenBSDBootstrapper(BaseBootstrapper):
    def __init__(self, version):
        BaseBootstrapper.__init__(self)

    def install_system_packages(self):
        
        self.run_as_root(['pkg_add', '-z',
            'mercurial',
            'llvm',
            'autoconf-2.13',
            'yasm',
            'gtk+2',
            'libIDL',
            'gmake',
            'gtar',
            'wget',
            'unzip',
            'zip'])
