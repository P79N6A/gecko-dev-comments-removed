





























"""WebSocket opening handshake processor. This class try to apply available
opening handshake processors for each protocol version until a connection is
successfully established.
"""


import logging

from mod_pywebsocket import common
from mod_pywebsocket.handshake import draft75
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
        allowDraft75: allow draft 75 handshake protocol.
        strict: Strictly check handshake request in draft 75.
            Default: False. If True, request.connection must provide
            get_memorized_lines method.

    Handshaker will add attributes such as ws_resource in performing
    handshake.
    """

    _LOGGER.debug('Client\'s opening handshake resource: %r', request.uri)
    
    
    
    
    
    
    
    
    
    
    
    
    _LOGGER.debug(
        'Client\'s opening handshake headers: %r', dict(request.headers_in))

    handshakers = []
    handshakers.append(
        ('IETF HyBi latest', hybi.Handshaker(request, dispatcher)))
    handshakers.append(
        ('IETF HyBi 00', hybi00.Handshaker(request, dispatcher)))
    if allowDraft75:
        handshakers.append(
            ('IETF Hixie 75', draft75.Handshaker(request, dispatcher, strict)))

    for name, handshaker in handshakers:
        _LOGGER.debug('Trying %s protocol', name)
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



