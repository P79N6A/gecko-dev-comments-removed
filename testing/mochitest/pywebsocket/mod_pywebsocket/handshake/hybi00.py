





























"""WebSocket initial handshake hander for HyBi 00 protocol."""









import logging
import re
import struct

from mod_pywebsocket import common
from mod_pywebsocket.stream import StreamHixie75
from mod_pywebsocket import util
from mod_pywebsocket.handshake._base import HandshakeError
from mod_pywebsocket.handshake._base import build_location
from mod_pywebsocket.handshake._base import check_header_lines
from mod_pywebsocket.handshake._base import get_mandatory_header
from mod_pywebsocket.handshake._base import validate_subprotocol


_MANDATORY_HEADERS = [
    
    [common.UPGRADE_HEADER, common.WEBSOCKET_UPGRADE_TYPE_HIXIE75],
    [common.CONNECTION_HEADER, common.UPGRADE_CONNECTION_TYPE],
]


class Handshaker(object):
    """This class performs WebSocket handshake."""

    def __init__(self, request, dispatcher):
        """Construct an instance.

        Args:
            request: mod_python request.
            dispatcher: Dispatcher (dispatch.Dispatcher).

        Handshaker will add attributes such as ws_resource in performing
        handshake.
        """

        self._logger = util.get_class_logger(self)

        self._request = request
        self._dispatcher = dispatcher

    def do_handshake(self):
        """Perform WebSocket Handshake.

        On _request, we set
            ws_resource, ws_protocol, ws_location, ws_origin, ws_challenge,
            ws_challenge_md5: WebSocket handshake information.
            ws_stream: Frame generation/parsing class.
            ws_version: Protocol version.
        """

        
        
        check_header_lines(self._request, _MANDATORY_HEADERS)
        self._set_resource()
        self._set_subprotocol()
        self._set_location()
        self._set_origin()
        self._set_challenge_response()
        self._set_protocol_version()
        self._dispatcher.do_extra_handshake(self._request)
        self._send_handshake()

    def _set_resource(self):
        self._request.ws_resource = self._request.uri

    def _set_subprotocol(self):
        
        subprotocol = self._request.headers_in.get(
            common.SEC_WEBSOCKET_PROTOCOL_HEADER)
        if subprotocol is not None:
            validate_subprotocol(subprotocol)
        self._request.ws_protocol = subprotocol

    def _set_location(self):
        
        host = self._request.headers_in.get(common.HOST_HEADER)
        if host is not None:
            self._request.ws_location = build_location(self._request)
        

    def _set_origin(self):
        
        origin = self._request.headers_in['Origin']
        if origin is not None:
            self._request.ws_origin = origin

    def _set_protocol_version(self):
        
        draft = self._request.headers_in.get('Sec-WebSocket-Draft')
        if draft is not None:
            try:
                draft_int = int(draft)

                
                
                
                

                if draft_int == 1 or draft_int == 2:
                    raise HandshakeError('HyBi 01-03 are not supported')
                elif draft_int != 0:
                    raise ValueError
            except ValueError, e:
                raise HandshakeError(
                    'Illegal value for Sec-WebSocket-Draft: %s' % draft)

        self._logger.debug('IETF HyBi 00 protocol')
        self._request.ws_version = common.VERSION_HYBI00
        self._request.ws_stream = StreamHixie75(self._request, True)

    def _set_challenge_response(self):
        
        self._request.ws_challenge = self._get_challenge()
        
        self._request.ws_challenge_md5 = util.md5_hash(
            self._request.ws_challenge).digest()
        self._logger.debug(
            'Challenge: %r (%s)' %
            (self._request.ws_challenge,
             util.hexify(self._request.ws_challenge)))
        self._logger.debug(
            'Challenge response: %r (%s)' %
            (self._request.ws_challenge_md5,
             util.hexify(self._request.ws_challenge_md5)))

    def _get_key_value(self, key_field):
        key_value = get_mandatory_header(self._request, key_field)

        
        
        
        
        try:
            key_number = int(re.sub("\\D", "", key_value))
        except:
            raise HandshakeError('%s field contains no digit' % key_field)
        
        
        spaces = re.subn(" ", "", key_value)[1]
        if spaces == 0:
            raise HandshakeError('%s field contains no space' % key_field)
        
        
        if key_number % spaces != 0:
            raise HandshakeError('Key-number %d is not an integral '
                                 'multiple of spaces %d' % (key_number,
                                                            spaces))
        
        part = key_number / spaces
        self._logger.debug('%s: %s => %d / %d => %d' % (
            key_field, key_value, key_number, spaces, part))
        return part

    def _get_challenge(self):
        
        key1 = self._get_key_value('Sec-WebSocket-Key1')
        key2 = self._get_key_value('Sec-WebSocket-Key2')
        
        challenge = ''
        challenge += struct.pack('!I', key1)  
        challenge += struct.pack('!I', key2)  
        challenge += self._request.connection.read(8)
        return challenge

    def _sendall(self, data):
        self._request.connection.write(data)

    def _send_header(self, name, value):
        self._sendall('%s: %s\r\n' % (name, value))

    def _send_handshake(self):
        
        self._sendall('HTTP/1.1 101 WebSocket Protocol Handshake\r\n')
        
        self._send_header(
            common.UPGRADE_HEADER, common.WEBSOCKET_UPGRADE_TYPE_HIXIE75)
        self._send_header(
            common.CONNECTION_HEADER, common.UPGRADE_CONNECTION_TYPE)
        self._send_header('Sec-WebSocket-Location', self._request.ws_location)
        self._send_header(
            common.SEC_WEBSOCKET_ORIGIN_HEADER, self._request.ws_origin)
        if self._request.ws_protocol:
            self._send_header(
                common.SEC_WEBSOCKET_PROTOCOL_HEADER,
                self._request.ws_protocol)
        
        self._sendall('\r\n')
        
        self._sendall(self._request.ws_challenge_md5)



