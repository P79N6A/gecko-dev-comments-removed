




class RunnerException(Exception):
    """Base exception handler for mozrunner related errors"""

class RunnerNotStartedError(RunnerException):
    """Exception handler in case the runner hasn't been started"""

class TimeoutException(RunnerException):
    """Raised on timeout waiting for targets to start."""

class ScriptTimeoutException(RunnerException):
    """Raised on timeout waiting for execute_script to finish."""
