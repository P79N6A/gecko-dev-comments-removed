





























"""WebSocket handshaking defined in draft-hixie-thewebsocketprotocol-75."""








import logging
import re

from mod_pywebsocket import common
from mod_pywebsocket.stream import StreamHixie75
from mod_pywebsocket import util
from mod_pywebsocket.handshake._base import HandshakeError
from mod_pywebsocket.handshake._base import build_location
from mod_pywebsocket.handshake._base import validate_subprotocol


_MANDATORY_HEADERS = [
    
    ['Upgrade', 'WebSocket'],
    ['Connection', 'Upgrade'],
    ['Host', None],
    ['Origin', None],
]

_FIRST_FIVE_LINES = map(re.compile, [
    r'^GET /[\S]* HTTP/1.1\r\n$',
    r'^Upgrade: WebSocket\r\n$',
    r'^Connection: Upgrade\r\n$',
    r'^Host: [\S]+\r\n$',
    r'^Origin: [\S]+\r\n$',
])

_SIXTH_AND_LATER = re.compile(
    r'^'
    r'(WebSocket-Protocol: [\x20-\x7e]+\r\n)?'
    r'(Cookie: [^\r]*\r\n)*'
    r'(Cookie2: [^\r]*\r\n)?'
    r'(Cookie: [^\r]*\r\n)*'
    r'\r\n')


class Handshaker(object):
    """This class performs WebSocket handshake."""

    def __init__(self, request, dispatcher, strict=False):
        """Construct an instance.

        Args:
            request: mod_python request.
            dispatcher: Dispatcher (dispatch.Dispatcher).
            strict: Strictly check handshake request. Default: False.
                If True, request.connection must provide get_memorized_lines
                method.

        Handshaker will add attributes such as ws_resource in performing
        handshake.
        """

        self._logger = util.get_class_logger(self)

        self._request = request
        self._dispatcher = dispatcher
        self._strict = strict

    def do_handshake(self):
        """Perform WebSocket Handshake.

        On _request, we set
            ws_resource, ws_origin, ws_location, ws_protocol
            ws_challenge_md5: WebSocket handshake information.
            ws_stream: Frame generation/parsing class.
            ws_version: Protocol version.
        """

        self._check_header_lines()
        self._set_resource()
        self._set_origin()
        self._set_location()
        self._set_subprotocol()
        self._set_protocol_version()
        self._dispatcher.do_extra_handshake(self._request)
        self._send_handshake()

    def _set_resource(self):
        self._request.ws_resource = self._request.uri

    def _set_origin(self):
        self._request.ws_origin = self._request.headers_in['Origin']

    def _set_location(self):
        self._request.ws_location = build_location(self._request)

    def _set_subprotocol(self):
        subprotocol = self._request.headers_in.get('WebSocket-Protocol')
        if subprotocol is not None:
            validate_subprotocol(subprotocol)
        self._request.ws_protocol = subprotocol

    def _set_protocol_version(self):
        self._logger.debug('IETF Hixie 75 protocol')
        self._request.ws_version = common.VERSION_HIXIE75
        self._request.ws_stream = StreamHixie75(self._request)

    def _sendall(self, data):
        self._request.connection.write(data)

    def _send_handshake(self):
        self._sendall('HTTP/1.1 101 Web Socket Protocol Handshake\r\n')
        self._sendall('Upgrade: WebSocket\r\n')
        self._sendall('Connection: Upgrade\r\n')
        self._sendall('WebSocket-Origin: %s\r\n' % self._request.ws_origin)
        self._sendall('WebSocket-Location: %s\r\n' % self._request.ws_location)
        if self._request.ws_protocol:
            self._sendall(
                'WebSocket-Protocol: %s\r\n' % self._request.ws_protocol)
        self._sendall('\r\n')

    def _check_header_lines(self):
        for key, expected_value in _MANDATORY_HEADERS:
            actual_value = self._request.headers_in.get(key)
            if not actual_value:
                raise HandshakeError('Header %s is not defined' % key)
            if expected_value:
                if actual_value != expected_value:
                    raise HandshakeError(
                        'Expected %r for header %s but found %r' %
                        (expected_value, key, actual_value))
        if self._strict:
            try:
                lines = self._request.connection.get_memorized_lines()
            except AttributeError, e:
                raise AttributeError(
                    'Strict handshake is specified but the connection '
                    'doesn\'t provide get_memorized_lines()')
            self._check_first_lines(lines)

    def _check_first_lines(self, lines):
        if len(lines) < len(_FIRST_FIVE_LINES):
            raise HandshakeError('Too few header lines: %d' % len(lines))
        for line, regexp in zip(lines, _FIRST_FIVE_LINES):
            if not regexp.search(line):
                raise HandshakeError('Unexpected header: %r doesn\'t match %r'
                                     % (line, regexp.pattern))
        sixth_and_later = ''.join(lines[5:])
        if not _SIXTH_AND_LATER.search(sixth_and_later):
            raise HandshakeError('Unexpected header: %r doesn\'t match %r'
                                 % (sixth_and_later,
                                    _SIXTH_AND_LATER.pattern))



