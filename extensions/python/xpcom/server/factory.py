







































import xpcom
from xpcom import components, nsError, _xpcom, logger

class Factory:
    _com_interfaces_ = components.interfaces.nsIFactory
    
    
    def __init__(self, klass):
        self.klass = klass

    def createInstance(self, outer, iid):
        if outer is not None:
            raise xpcom.ServerException(nsError.NS_ERROR_NO_AGGREGATION)

        logger.debug("Python Factory creating %s", self.klass.__name__)
        try:
            return self.klass()
        except:
            
            
            
            logger.error("Creation of class '%r' failed!\nException details follow\n",
                           self.klass)
            
            raise

    def lockServer(self, lock):
        logger.debug("Python Factory LockServer called '%s'", lock)
