





























"""PythonHeaderParserHandler for mod_pywebsocket.

Apache HTTP Server and mod_python must be configured such that this
function is called to handle WebSocket request.
"""


import logging

from mod_python import apache

from mod_pywebsocket import dispatch
from mod_pywebsocket import handshake
from mod_pywebsocket import util



_PYOPT_HANDLER_ROOT = 'mod_pywebsocket.handler_root'




_PYOPT_HANDLER_SCAN = 'mod_pywebsocket.handler_scan'



_PYOPT_ALLOW_DRAFT75 = 'mod_pywebsocket.allow_draft75'


class ApacheLogHandler(logging.Handler):
    """Wrapper logging.Handler to emit log message to apache's error.log."""

    _LEVELS = {
        logging.DEBUG: apache.APLOG_DEBUG,
        logging.INFO: apache.APLOG_INFO,
        logging.WARNING: apache.APLOG_WARNING,
        logging.ERROR: apache.APLOG_ERR,
        logging.CRITICAL: apache.APLOG_CRIT,
        }

    def __init__(self, request=None):
        logging.Handler.__init__(self)
        self.log_error = apache.log_error
        if request is not None:
            self.log_error = request.log_error

    def emit(self, record):
        apache_level = apache.APLOG_DEBUG
        if record.levelno in ApacheLogHandler._LEVELS:
            apache_level = ApacheLogHandler._LEVELS[record.levelno]
        self.log_error(record.getMessage(), apache_level)


logging.getLogger('mod_pywebsocket').addHandler(ApacheLogHandler())


def _create_dispatcher():
    _HANDLER_ROOT = apache.main_server.get_options().get(
            _PYOPT_HANDLER_ROOT, None)
    if not _HANDLER_ROOT:
        raise Exception('PythonOption %s is not defined' % _PYOPT_HANDLER_ROOT,
                        apache.APLOG_ERR)
    _HANDLER_SCAN = apache.main_server.get_options().get(
            _PYOPT_HANDLER_SCAN, _HANDLER_ROOT)
    dispatcher = dispatch.Dispatcher(_HANDLER_ROOT, _HANDLER_SCAN)
    for warning in dispatcher.source_warnings():
        apache.log_error('mod_pywebsocket: %s' % warning, apache.APLOG_WARNING)
    return dispatcher



_dispatcher = _create_dispatcher()


def headerparserhandler(request):
    """Handle request.

    Args:
        request: mod_python request.

    This function is named headerparserhandler because it is the default name
    for a PythonHeaderParserHandler.
    """

    try:
        allowDraft75 = apache.main_server.get_options().get(
            _PYOPT_ALLOW_DRAFT75, None)
        handshaker = handshake.Handshaker(request, _dispatcher,
                                          allowDraft75=allowDraft75)
        handshaker.do_handshake()
        request.log_error(
            'mod_pywebsocket: resource: %r' % request.ws_resource,
            apache.APLOG_DEBUG)
        try:
            _dispatcher.transfer_data(request)
        except Exception, e:
            
            
            
            request.log_error('mod_pywebsocket: %s' % e, apache.APLOG_WARNING)
    except handshake.HandshakeError, e:
        
        
        request.log_error('mod_pywebsocket: %s' % e, apache.APLOG_INFO)
        return apache.DECLINED
    except dispatch.DispatchError, e:
        request.log_error('mod_pywebsocket: %s' % e, apache.APLOG_WARNING)
        return apache.DECLINED
    
    request.assbackwards = 1
    return apache.DONE  



