




































class MarionetteException(Exception):

    def __init__(self, message=None, status=500, stacktrace=None):
        self.message = message
        self.status = status
        self.stacktrace = stacktrace

    def __str__(self):
        if self.stacktrace:
            return '%s\nstacktrace:\n%s' % (self.message,
                ''.join(['\t%s\n' % x for x in self.stacktrace.split('\n')]))
        else:
            return self.message

class TimeoutException(MarionetteException):
    pass

class JavascriptException(MarionetteException):
    pass

class NoSuchElementException(MarionetteException):
    pass

class XPathLookupException(MarionetteException):
    pass

class NoSuchWindowException(MarionetteException):
    pass

class StaleElementException(MarionetteException):
    pass

class ScriptTimeoutException(MarionetteException):
    pass

class ElementNotVisibleException(MarionetteException):
    pass

class NoSuchFrameException(MarionetteException):
    pass

