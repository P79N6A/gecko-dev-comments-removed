



class BaseBootstrapper(object):
    """Base class for system bootstrappers."""
    def __init__(self):
        pass

    def install_system_packages(self):
        raise NotImplemented('%s must implement install_system_packages()' %
            __name__)
