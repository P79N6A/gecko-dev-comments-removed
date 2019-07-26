



import os

class BaseBootstrapper(object):
    """Base class for system bootstrappers."""
    def __init__(self):
        pass

    def install_system_packages(self):
        raise NotImplemented('%s must implement install_system_packages()' %
            __name__)

    def which(self, name):
        """Python implementation of which.

        It returns the path of an executable or None if it couldn't be found.
        """
        for path in os.environ['PATH'].split(os.pathsep):
            test = os.path.join(path, name)
            if os.path.exists(test) and os.access(test, os.X_OK):
                return test

        return None
