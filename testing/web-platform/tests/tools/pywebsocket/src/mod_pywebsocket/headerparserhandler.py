





























"""PythonHeaderParserHandler for mod_pywebsocket.

Apache HTTP Server and mod_python must be configured such that this
function is called to handle WebSocket request.
"""


import logging

from mod_python import apache

from mod_pywebsocket import common
from mod_pywebsocket import dispatch
from mod_pywebsocket import handshake
from mod_pywebsocket import util



_PYOPT_HANDLER_ROOT = 'mod_pywebsocket.handler_root'




_PYOPT_HANDLER_SCAN = 'mod_pywebsocket.handler_scan'




_PYOPT_ALLOW_HANDLERS_OUTSIDE_ROOT = (
    'mod_pywebsocket.allow_handlers_outside_root_dir')


_PYOPT_ALLOW_HANDLERS_OUTSIDE_ROOT_DEFINITION = {
    'off': False, 'no': False, 'on': True, 'yes': True}




_PYOPT_ALLOW_DRAFT75 = 'mod_pywebsocket.allow_draft75'

_PYOPT_ALLOW_DRAFT75_DEFINITION = {'off': False, 'on': True}


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
        self._log_error = apache.log_error
        if request is not None:
            self._log_error = request.log_error

        
        self._formatter = logging.Formatter('%(name)s: %(message)s')

    def emit(self, record):
        apache_level = apache.APLOG_DEBUG
        if record.levelno in ApacheLogHandler._LEVELS:
            apache_level = ApacheLogHandler._LEVELS[record.levelno]

        msg = self._formatter.format(record)

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        self._log_error(msg, apache_level, apache.main_server)


def _configure_logging():
    logger = logging.getLogger()
    
    
    
    logger.setLevel(logging.DEBUG)
    logger.addHandler(ApacheLogHandler())


_configure_logging()

_LOGGER = logging.getLogger(__name__)


def _parse_option(name, value, definition):
    if value is None:
        return False

    meaning = definition.get(value.lower())
    if meaning is None:
        raise Exception('Invalid value for PythonOption %s: %r' %
                        (name, value))
    return meaning


def _create_dispatcher():
    _LOGGER.info('Initializing Dispatcher')

    options = apache.main_server.get_options()

    handler_root = options.get(_PYOPT_HANDLER_ROOT, None)
    if not handler_root:
        raise Exception('PythonOption %s is not defined' % _PYOPT_HANDLER_ROOT,
                        apache.APLOG_ERR)

    handler_scan = options.get(_PYOPT_HANDLER_SCAN, handler_root)

    allow_handlers_outside_root = _parse_option(
        _PYOPT_ALLOW_HANDLERS_OUTSIDE_ROOT,
        options.get(_PYOPT_ALLOW_HANDLERS_OUTSIDE_ROOT),
        _PYOPT_ALLOW_HANDLERS_OUTSIDE_ROOT_DEFINITION)

    dispatcher = dispatch.Dispatcher(
        handler_root, handler_scan, allow_handlers_outside_root)

    for warning in dispatcher.source_warnings():
        apache.log_error(
            'mod_pywebsocket: Warning in source loading: %s' % warning,
            apache.APLOG_WARNING)

    return dispatcher



_dispatcher = _create_dispatcher()


def headerparserhandler(request):
    """Handle request.

    Args:
        request: mod_python request.

    This function is named headerparserhandler because it is the default
    name for a PythonHeaderParserHandler.
    """

    handshake_is_done = False
    try:
        
        
        if not _dispatcher.get_handler_suite(request.uri):
            request.log_error(
                'mod_pywebsocket: No handler for resource: %r' % request.uri,
                apache.APLOG_INFO)
            request.log_error(
                'mod_pywebsocket: Fallback to Apache', apache.APLOG_INFO)
            return apache.DECLINED
    except dispatch.DispatchException, e:
        request.log_error(
            'mod_pywebsocket: Dispatch failed for error: %s' % e,
            apache.APLOG_INFO)
        if not handshake_is_done:
            return e.status

    try:
        allow_draft75 = _parse_option(
            _PYOPT_ALLOW_DRAFT75,
            apache.main_server.get_options().get(_PYOPT_ALLOW_DRAFT75),
            _PYOPT_ALLOW_DRAFT75_DEFINITION)

        try:
            handshake.do_handshake(
                request, _dispatcher, allowDraft75=allow_draft75)
        except handshake.VersionException, e:
            request.log_error(
                'mod_pywebsocket: Handshake failed for version error: %s' % e,
                apache.APLOG_INFO)
            request.err_headers_out.add(common.SEC_WEBSOCKET_VERSION_HEADER,
                                        e.supported_versions)
            return apache.HTTP_BAD_REQUEST
        except handshake.HandshakeException, e:
            
            
            request.log_error(
                'mod_pywebsocket: Handshake failed for error: %s' % e,
                apache.APLOG_INFO)
            return e.status

        handshake_is_done = True
        request._dispatcher = _dispatcher
        _dispatcher.transfer_data(request)
    except handshake.AbortedByUserException, e:
        request.log_error('mod_pywebsocket: Aborted: %s' % e, apache.APLOG_INFO)
    except Exception, e:
        
        

        request.log_error('mod_pywebsocket: Exception occurred: %s\n%s' %
                          (e, util.get_stack_trace()),
                          apache.APLOG_ERR)
        
        
        if not handshake_is_done:
            return apache.DECLINED
    
    request.assbackwards = 1
    return apache.DONE  



