





























"""WebSocket opening handshake processor. This class try to apply available
opening handshake processors for each protocol version until a connection is
successfully established.
"""


import logging

from mod_pywebsocket import util
from mod_pywebsocket.handshake import draft75
from mod_pywebsocket.handshake import hybi00
from mod_pywebsocket.handshake import hybi06
from mod_pywebsocket.handshake._base import HandshakeError


class Handshaker(object):
    """This class performs WebSocket handshake."""

    def __init__(self, request, dispatcher, allowDraft75=False, strict=False):
        """Construct an instance.

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

        self._logger = util.get_class_logger(self)

        self._request = request
        self._dispatcher = dispatcher
        self._strict = strict
        self._hybi07Handshaker = hybi06.Handshaker(request, dispatcher)
        self._hybi00Handshaker = hybi00.Handshaker(request, dispatcher)
        self._hixie75Handshaker = None
        if allowDraft75:
            self._hixie75Handshaker = draft75.Handshaker(
                request, dispatcher, strict)

    def do_handshake(self):
        """Perform WebSocket Handshake."""

        self._logger.debug(
            'Opening handshake headers: %s' % self._request.headers_in)

        handshakers = [
            ('HyBi 07', self._hybi07Handshaker),
            ('HyBi 00', self._hybi00Handshaker),
            ('Hixie 75', self._hixie75Handshaker)]
        last_error = HandshakeError('No handshaker available')
        for name, handshaker in handshakers:
            if handshaker:
                self._logger.info('Trying %s protocol' % name)
                try:
                    handshaker.do_handshake()
                    return
                except HandshakeError, e:
                    self._logger.info('%s handshake failed: %s' % (name, e))
                    last_error = e
        raise last_error


