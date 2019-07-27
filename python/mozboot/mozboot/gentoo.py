



from mozboot.base import BaseBootstrapper


class GentooBootstrapper(BaseBootstrapper):
    def __init__(self, version, dist_id, **kwargs):
        BaseBootstrapper.__init__(self, **kwargs)

        self.version = version
        self.dist_id = dist_id

    def install_system_packages(self):
        self.run_as_root(['emerge', '--quiet', 'git', 'mercurial'])

    def install_browser_packages(self):
        self.run_as_root(['emerge', '--onlydeps', '--quiet', 'firefox'])

    def _update_package_manager(self):
        self.run_as_root(['emerge', '--sync'])

    def upgrade_mercurial(self, current):
        self.run_as_root(['emerge', '--update', 'mercurial'])
