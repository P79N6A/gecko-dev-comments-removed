









































import xpcom.server
from xpcom import _xpcom
from xpcom.components import interfaces

import logging

_handlers = []

class _ShutdownObserver:
    _com_interfaces_ = interfaces.nsIObserver
    def observe(self, service, topic, extra):
        logger = logging.getLogger('xpcom')
        while _handlers:
            func, args, kw = _handlers.pop()
            try:
                logger.debug("Calling shutdown handler '%s'(*%s, **%s)",
                             func, args, kw)
                func(*args, **kw)
            except:
                logger.exception("Shutdown handler '%s' failed", func)

def register(func, *args, **kw):
    _handlers.append( (func, args, kw) )


svc = _xpcom.GetServiceManager().getServiceByContractID(
                                    "@mozilla.org/observer-service;1",
                                    interfaces.nsIObserverService)

svc.addObserver(_ShutdownObserver(), "xpcom-shutdown", 0)

del svc, _ShutdownObserver
