






































import exceptions



verbose = 0

hr_map = {}



class Exception(exceptions.Exception):
    def __init__(self, errno, message = None):
        assert int(errno) == errno, "The errno param must be an integer"
        self.errno = errno
        self.message = message
        exceptions.Exception.__init__(self, errno)
    def __str__(self):
        if not hr_map:
            import nsError
            for name, val in nsError.__dict__.items():
                if type(val)==type(0):
                    hr_map[val] = name
        message = self.message
        if message is None:
            message = hr_map.get(self.errno)
            if message is None:
                message = ""
        return "%d (%s)" % (self.errno, message)



COMException = Exception







class ServerException(Exception):
    def __init__(self, errno=None, *args, **kw):
        if errno is None:
            import nsError
            errno = nsError.NS_ERROR_FAILURE
        Exception.__init__(self, errno, *args, **kw)









import logging
class ConsoleServiceStream:
    
    def flush(self):
        pass
    def write(self, msg):
        import _xpcom
        _xpcom.LogConsoleMessage(msg)
    def close(self):
        pass

def setupLogging():
    import sys, os, threading, thread
    hdlr = logging.StreamHandler(ConsoleServiceStream())
    fmt = logging.Formatter(logging.BASIC_FORMAT)
    hdlr.setFormatter(fmt)
    
    
    
    
    
    if type(hdlr.lock) == thread.LockType:
        hdlr.lock = threading.RLock()

    logger.addHandler(hdlr)
    
    
    
    filename = os.environ.get("PYXPCOM_LOG_FILE")
    stream = sys.stderr 
    if filename:
        try:
            
            stream = open(filename, "wU", 0)
        except IOError, why:
            print >> sys.stderr, "pyxpcom failed to open log file '%s': %s" \
                                 % (filename, why)
            

    hdlr = logging.StreamHandler(stream)
    
    if type(hdlr.lock) == thread.LockType:
        hdlr.lock = threading.RLock()

    fmt = logging.Formatter(logging.BASIC_FORMAT)
    hdlr.setFormatter(fmt)
    logger.addHandler(hdlr)
    
    level = os.environ.get("PYXPCOM_LOG_LEVEL")
    if level:
        try:
            level = int(level)
        except ValueError:
            try:
                
                level = int(getattr(logging, level.upper()))
            except (AttributeError, ValueError):
                logger.warning("The PYXPCOM_LOG_LEVEL variable specifies an "
                               "invalid level")
                level = None
        if level:
            logger.setLevel(level)

logger = logging.getLogger('xpcom')

if len(logger.handlers) == 0:
    setupLogging()



del ConsoleServiceStream, logging, setupLogging
