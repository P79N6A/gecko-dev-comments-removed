



import os

from mozboot.debian import DebianBootstrapper


MERCURIAL_PPA = '''
Ubuntu does not provide a modern Mercurial in its package repository. So,
we will install a PPA that does.
'''.strip()


class UbuntuBootstrapper(DebianBootstrapper):
    DISTRO_PACKAGES = [
        
        'software-properties-common',
    ]

    def upgrade_mercurial(self, current):
        
        
        self._add_ppa('mercurial-ppa/releases')
        self._update_package_manager()
        self.apt_install('mercurial')

    def _add_ppa(self, ppa):
        
        
        
        list_file = ppa.replace('/', '-')
        for source in os.listdir('/etc/apt/sources.list.d'):
            if source.startswith(list_file) and source.endswith('.list'):
                return

        self.run_as_root(['add-apt-repository', 'ppa:%s' % ppa])
