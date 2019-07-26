




class RunnerException(Exception):
    """Base exception handler for mozrunner related errors"""


class RunnerNotStartedError(RunnerException):
    """Exception handler in case the runner hasn't been started"""
