





























"""WebSocket HyBi 07 opening handshake processor."""








import base64
import logging
import os
import re

from mod_pywebsocket import common
from mod_pywebsocket.stream import Stream
from mod_pywebsocket.stream import StreamOptions
from mod_pywebsocket import util
from mod_pywebsocket.handshake._base import check_header_lines
from mod_pywebsocket.handshake._base import Extension
from mod_pywebsocket.handshake._base import format_extensions
from mod_pywebsocket.handshake._base import format_header
from mod_pywebsocket.handshake._base import get_mandatory_header
from mod_pywebsocket.handshake._base import HandshakeError
from mod_pywebsocket.handshake._base import parse_extensions
from mod_pywebsocket.handshake._base import validate_mandatory_header


_MANDATORY_HEADERS = [
    
    [common.UPGRADE_HEADER, common.WEBSOCKET_UPGRADE_TYPE],
    [common.CONNECTION_HEADER, common.UPGRADE_CONNECTION_TYPE],
]

_BASE64_REGEX = re.compile('^[+/0-9A-Za-z]*=*$')


def compute_accept(key):
    """Computes value for the Sec-WebSocket-Accept header from value of the
    Sec-WebSocket-Key header.
    """

    accept_binary = util.sha1_hash(
        key + common.WEBSOCKET_ACCEPT_UUID).digest()
    accept = base64.b64encode(accept_binary)

    return (accept, accept_binary)


class Handshaker(object):
    """This class performs WebSocket handshake."""

    def __init__(self, request, dispatcher):
        """Construct an instance.

        Args:
            request: mod_python request.
            dispatcher: Dispatcher (dispatch.Dispatcher).

        Handshaker will add attributes such as ws_resource during handshake.
        """

        self._logger = util.get_class_logger(self)

        self._request = request
        self._dispatcher = dispatcher

    def do_handshake(self):
        check_header_lines(self._request, _MANDATORY_HEADERS)
        self._request.ws_resource = self._request.uri

        unused_host = get_mandatory_header(self._request, common.HOST_HEADER)

        self._get_origin()
        self._check_version()
        self._set_protocol()
        self._set_extensions()

        key = self._get_key()
        (accept, accept_binary) = compute_accept(key)
        self._logger.debug('Sec-WebSocket-Accept: %r (%s)',
                           accept, util.hexify(accept_binary))

        self._logger.debug('IETF HyBi 07 protocol')
        self._request.ws_version = common.VERSION_HYBI07
        stream_options = StreamOptions()
        stream_options.deflate = self._request.ws_deflate
        self._request.ws_stream = Stream(self._request, stream_options)

        self._request.ws_close_code = None
        self._request.ws_close_reason = None

        self._dispatcher.do_extra_handshake(self._request)

        if self._request.ws_requested_protocols is not None:
            if self._request.ws_protocol is None:
                raise HandshakeError(
                    'do_extra_handshake must choose one subprotocol from '
                    'ws_requested_protocols and set it to ws_protocol')

            

            self._logger.debug(
                'Subprotocol accepted: %r',
                self._request.ws_protocol)
        else:
            if self._request.ws_protocol is not None:
                raise HandshakeError(
                    'ws_protocol must be None when the client didn\'t request '
                    'any subprotocol')

        self._send_handshake(accept)

        self._logger.debug('Sent opening handshake response')

    def _get_origin(self):
        origin = self._request.headers_in.get(
            common.SEC_WEBSOCKET_ORIGIN_HEADER)
        self._request.ws_origin = origin

    def _check_version(self):
        unused_value = validate_mandatory_header(
            self._request, common.SEC_WEBSOCKET_VERSION_HEADER, '8')

    def _set_protocol(self):
        self._request.ws_protocol = None

        protocol_header = self._request.headers_in.get(
            common.SEC_WEBSOCKET_PROTOCOL_HEADER)

        if not protocol_header:
            self._request.ws_requested_protocols = None
            return

        

        requested_protocols = protocol_header.split(',')
        self._request.ws_requested_protocols = [
            s.strip() for s in requested_protocols]

        self._logger.debug('Subprotocols requested: %r', requested_protocols)

    def _set_extensions(self):
        self._request.ws_deflate = False

        extensions_header = self._request.headers_in.get(
            common.SEC_WEBSOCKET_EXTENSIONS_HEADER)
        if not extensions_header:
            self._request.ws_requested_extensions = None
            self._request.ws_extensions = None
            return

        self._request.ws_extensions = []

        requested_extensions = parse_extensions(extensions_header)

        for extension in requested_extensions:
            extension_name = extension.name()
            
            
            if (extension_name == 'deflate-stream' and
                len(extension.get_parameter_names()) == 0):
                self._request.ws_extensions.append(extension)
                self._request.ws_deflate = True

        self._request.ws_requested_extensions = requested_extensions

        self._logger.debug(
            'Extensions requested: %r',
            map(Extension.name, self._request.ws_requested_extensions))
        self._logger.debug(
            'Extensions accepted: %r',
            map(Extension.name, self._request.ws_extensions))

    def _validate_key(self, key):
        
        key_is_valid = False
        try:
            
            
            
            
            if _BASE64_REGEX.match(key):
                decoded_key = base64.b64decode(key)
                if len(decoded_key) == 16:
                    key_is_valid = True
        except TypeError, e:
            pass

        if not key_is_valid:
            raise HandshakeError(
                'Illegal value for header %s: %r' %
                (common.SEC_WEBSOCKET_KEY_HEADER, key))

        return decoded_key

    def _get_key(self):
        key = get_mandatory_header(
            self._request, common.SEC_WEBSOCKET_KEY_HEADER)

        decoded_key = self._validate_key(key)

        self._logger.debug('Sec-WebSocket-Key: %r (%s)',
                           key, util.hexify(decoded_key))

        return key

    def _send_handshake(self, accept):
        response = []

        response.append('HTTP/1.1 101 Switching Protocols\r\n')

        response.append(format_header(
            common.UPGRADE_HEADER, common.WEBSOCKET_UPGRADE_TYPE))
        response.append(format_header(
            common.CONNECTION_HEADER, common.UPGRADE_CONNECTION_TYPE))
        response.append(format_header(
            common.SEC_WEBSOCKET_ACCEPT_HEADER, accept))
        
        
        if self._request.ws_protocol is not None:
            response.append(format_header(
                common.SEC_WEBSOCKET_PROTOCOL_HEADER,
                self._request.ws_protocol))
        if self._request.ws_extensions is not None:
            response.append(format_header(
                common.SEC_WEBSOCKET_EXTENSIONS_HEADER,
                format_extensions(self._request.ws_extensions)))
        response.append('\r\n')

        raw_response = ''.join(response)
        self._logger.debug('Opening handshake response: %r', raw_response)
        self._request.connection.write(raw_response)



