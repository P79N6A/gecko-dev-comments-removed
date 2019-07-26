



from mozboot.base import BaseBootstrapper

class UbuntuBootstrapper(BaseBootstrapper):
    def __init__(self, version, dist_id):
        BaseBootstrapper.__init__(self)

        self.version = version
        self.dist_id = dist_id

    def install_system_packages(self):
        raise NotImplementedError('Bootstrap for Ubuntu not yet implemented.')
