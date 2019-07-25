





























"""Web Socket handshaking.

Note: request.connection.write/read are used in this module, even though
mod_python document says that they should be used only in connection handlers.
Unfortunately, we have no other options. For example, request.write/read are
not suitable because they don't allow direct raw bytes writing/reading.
"""


import logging
import re

from mod_pywebsocket.handshake import draft75
from mod_pywebsocket.handshake import handshake
from mod_pywebsocket.handshake._base import DEFAULT_WEB_SOCKET_PORT
from mod_pywebsocket.handshake._base import DEFAULT_WEB_SOCKET_SECURE_PORT
from mod_pywebsocket.handshake._base import WEB_SOCKET_SCHEME
from mod_pywebsocket.handshake._base import WEB_SOCKET_SECURE_SCHEME
from mod_pywebsocket.handshake._base import HandshakeError
from mod_pywebsocket.handshake._base import validate_protocol


class Handshaker(object):
    """This class performs Web Socket handshake."""

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

        self._logger = logging.getLogger("mod_pywebsocket.handshake")
        self._request = request
        self._dispatcher = dispatcher
        self._strict = strict
        self._handshaker = handshake.Handshaker(request, dispatcher)
        self._fallbackHandshaker = None
        if allowDraft75:
            self._fallbackHandshaker = draft75.Handshaker(
                request, dispatcher, strict)

    def do_handshake(self):
        """Perform Web Socket Handshake."""

        try:
            self._handshaker.do_handshake()
        except HandshakeError, e:
            self._logger.error('Handshake error: %s' % e)
            if self._fallbackHandshaker:
                self._logger.warning('fallback to old protocol')
                self._fallbackHandshaker.do_handshake()
                return
            raise e



