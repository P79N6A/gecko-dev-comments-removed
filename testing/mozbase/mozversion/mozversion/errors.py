




class VersionError(Exception):
    def __init__(self, message):
        Exception.__init__(self, message)


class AppNotFoundError(VersionError):
    """Exception for the application not found"""
    def __init__(self, message):
        VersionError.__init__(self, message)


class LocalAppNotFoundError(AppNotFoundError):
    """Exception for local application not found"""
    def __init__(self, path):
        AppNotFoundError.__init__(self, 'Application not found at: %s' % path)


class RemoteAppNotFoundError(AppNotFoundError):
    """Exception for remote application not found"""
    def __init__(self, message):
        AppNotFoundError.__init__(self, message)
