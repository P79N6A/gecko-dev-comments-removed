





























"""Web Socket handshaking.

Note: request.connection.write/read are used in this module, even though
mod_python document says that they should be used only in connection handlers.
Unfortunately, we have no other options. For example, request.write/read are
not suitable because they don't allow direct raw bytes writing/reading.
"""


import logging
from md5 import md5
import re
import struct

from mod_pywebsocket.handshake._base import HandshakeError
from mod_pywebsocket.handshake._base import build_location
from mod_pywebsocket.handshake._base import validate_protocol


_MANDATORY_HEADERS = [
    
    ['Upgrade', 'WebSocket'],
    ['Connection', 'Upgrade'],
]

def _hexify(s):
    return re.sub('.', lambda x: '%02x ' % ord(x.group(0)), s)

class Handshaker(object):
    """This class performs Web Socket handshake."""

    def __init__(self, request, dispatcher):
        """Construct an instance.

        Args:
            request: mod_python request.
            dispatcher: Dispatcher (dispatch.Dispatcher).

        Handshaker will add attributes such as ws_resource in performing
        handshake.
        """

        self._logger = logging.getLogger("mod_pywebsocket.handshake")
        self._request = request
        self._dispatcher = dispatcher

    def do_handshake(self):
        """Perform Web Socket Handshake."""

        
        
        self._check_header_lines()
        self._set_resource()
        self._set_protocol()
        self._set_location()
        self._set_origin()
        self._set_challenge_response()
        self._dispatcher.do_extra_handshake(self._request)
        self._send_handshake()

    def _check_header_lines(self):
        
        
        if self._request.method != 'GET':
            raise HandshakeError('Method is not GET')
        
        
        
        for key, expected_value in _MANDATORY_HEADERS:
            actual_value = self._request.headers_in.get(key)
            if not actual_value:
                raise HandshakeError('Header %s is not defined' % key)
            if expected_value:
                if actual_value != expected_value:
                    raise HandshakeError('Illegal value for header %s: %s' %
                                         (key, actual_value))

    def _set_resource(self):
        self._request.ws_resource = self._request.uri

    def _set_protocol(self):
        
        protocol = self._request.headers_in.get('Sec-WebSocket-Protocol')
        if protocol is not None:
            validate_protocol(protocol)
        self._request.ws_protocol = protocol

    def _set_location(self):
        
        host = self._request.headers_in.get('Host')
        if host is not None:
            self._request.ws_location = build_location(self._request)
        

    def _set_origin(self):
        
        origin = self._request.headers_in['Origin']
        if origin is not None:
            self._request.ws_origin = origin

    def _set_challenge_response(self):
        
        self._request.ws_challenge = self._get_challenge()
        
        self._request.ws_challenge_md5 = md5(
            self._request.ws_challenge).digest()
        self._logger.debug("challenge: %s" % _hexify(
            self._request.ws_challenge))
        self._logger.debug("response:  %s" % _hexify(
            self._request.ws_challenge_md5))

    def _get_key_value(self, key_field):
        key_value = self._request.headers_in.get(key_field)
        if key_value is None:
            self._logger.debug("no %s" % key_value)
            return None
        try:
            
            
            
            
            key_number = int(re.sub("\\D", "", key_value))
            
            
            spaces = re.subn(" ", "", key_value)[1]
            
            
            if key_number % spaces != 0:
                raise handshakeError('key_number %d is not an integral '
                                     'multiple of spaces %d' % (key_number,
                                                                spaces))
            
            part = key_number / spaces
            self._logger.debug("%s: %s => %d / %d => %d" % (
                key_field, key_value, key_number, spaces, part))
            return part
        except:
            return None

    def _get_challenge(self):
        
        key1 = self._get_key_value('Sec-Websocket-Key1')
        if not key1:
            raise HandshakeError('Sec-WebSocket-Key1 not found')
        key2 = self._get_key_value('Sec-Websocket-Key2')
        if not key2:
            raise HandshakeError('Sec-WebSocket-Key2 not found')
        
        challenge = ""
        challenge += struct.pack("!I", key1)  
        challenge += struct.pack("!I", key2)  
        challenge += self._request.connection.read(8)
        return challenge

    def _send_handshake(self):
        
        self._request.connection.write(
                'HTTP/1.1 101 WebSocket Protocol Handshake\r\n')
        
        self._request.connection.write('Upgrade: WebSocket\r\n')
        self._request.connection.write('Connection: Upgrade\r\n')
        self._request.connection.write('Sec-WebSocket-Location: ')
        self._request.connection.write(self._request.ws_location)
        self._request.connection.write('\r\n')
        self._request.connection.write('Sec-WebSocket-Origin: ')
        self._request.connection.write(self._request.ws_origin)
        self._request.connection.write('\r\n')
        if self._request.ws_protocol:
            self._request.connection.write('Sec-WebSocket-Protocol: ')
            self._request.connection.write(self._request.ws_protocol)
            self._request.connection.write('\r\n')
        
        self._request.connection.write('\r\n')
        
        self._request.connection.write(self._request.ws_challenge_md5)



