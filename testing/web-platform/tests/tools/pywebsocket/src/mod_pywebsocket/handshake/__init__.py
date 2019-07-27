





























"""WebSocket opening handshake processor. This class try to apply available
opening handshake processors for each protocol version until a connection is
successfully established.
"""


import logging

from mod_pywebsocket import common
from mod_pywebsocket.handshake import hybi00
from mod_pywebsocket.handshake import hybi


from mod_pywebsocket.handshake._base import AbortedByUserException
from mod_pywebsocket.handshake._base import HandshakeException
from mod_pywebsocket.handshake._base import VersionException


_LOGGER = logging.getLogger(__name__)


def do_handshake(request, dispatcher, allowDraft75=False, strict=False):
    """Performs WebSocket handshake.

    Args:
        request: mod_python request.
        dispatcher: Dispatcher (dispatch.Dispatcher).
        allowDraft75: obsolete argument. ignored.
        strict: obsolete argument. ignored.

    Handshaker will add attributes such as ws_resource in performing
    handshake.
    """

    _LOGGER.debug('Client\'s opening handshake resource: %r', request.uri)
    
    
    
    
    
    
    
    
    
    
    
    
    _LOGGER.debug(
        'Client\'s opening handshake headers: %r', dict(request.headers_in))

    handshakers = []
    handshakers.append(
        ('RFC 6455', hybi.Handshaker(request, dispatcher)))
    handshakers.append(
        ('HyBi 00', hybi00.Handshaker(request, dispatcher)))

    for name, handshaker in handshakers:
        _LOGGER.debug('Trying protocol version %s', name)
        try:
            handshaker.do_handshake()
            _LOGGER.info('Established (%s protocol)', name)
            return
        except HandshakeException, e:
            _LOGGER.debug(
                'Failed to complete opening handshake as %s protocol: %r',
                name, e)
            if e.status:
                raise e
        except AbortedByUserException, e:
            raise
        except VersionException, e:
            raise

    
    raise HandshakeException(
        'Failed to complete opening handshake for all available protocols',
        status=common.HTTP_STATUS_BAD_REQUEST)



