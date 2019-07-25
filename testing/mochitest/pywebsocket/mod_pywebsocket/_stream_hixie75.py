





























"""Stream of WebSocket protocol with the framing used by IETF HyBi 00 and
Hixie 75. For Hixie 75 this stream doesn't perform closing handshake.
"""


from mod_pywebsocket import common
from mod_pywebsocket._stream_base import BadOperationException
from mod_pywebsocket._stream_base import ConnectionTerminatedException
from mod_pywebsocket._stream_base import InvalidFrameException
from mod_pywebsocket._stream_base import StreamBase
from mod_pywebsocket._stream_base import UnsupportedFrameException
from mod_pywebsocket import util


class StreamHixie75(StreamBase):
    """Stream of WebSocket messages."""

    def __init__(self, request, enable_closing_handshake=False):
        """Construct an instance.

        Args:
            request: mod_python request.
            enable_closing_handshake: to let StreamHixie75 perform closing
                                      handshake as specified in HyBi 00, set
                                      this option to True.
        """

        StreamBase.__init__(self, request)

        self._logger = util.get_class_logger(self)

        self._enable_closing_handshake = enable_closing_handshake

        self._request.client_terminated = False
        self._request.server_terminated = False

    def send_message(self, message, end=True):
        """Send message.

        Args:
            message: unicode string to send.

        Raises:
            BadOperationException: when called on a server-terminated
                connection.
        """

        if not end:
            raise BadOperationException(
                'StreamHixie75 doesn\'t support send_message with end=False')

        if self._request.server_terminated:
            raise BadOperationException(
                'Requested send_message after sending out a closing handshake')

        self._write(''.join(['\x00', message.encode('utf-8'), '\xff']))

    def _read_payload_length_hixie75(self):
        """Reads a length header in a Hixie75 version frame with length.

        Raises:
            ConnectionTerminatedException: when read returns empty string.
        """

        length = 0
        while True:
            b_str = self._read(1)
            b = ord(b_str)
            length = length * 128 + (b & 0x7f)
            if (b & 0x80) == 0:
                break
        return length

    def receive_message(self):
        """Receive a WebSocket frame and return its payload an unicode string.

        Returns:
            payload unicode string in a WebSocket frame.

        Raises:
            ConnectionTerminatedException: when read returns empty
                string.
            BadOperationException: when called on a client-terminated
                connection.
        """

        if self._request.client_terminated:
            raise BadOperationException(
                'Requested receive_message after receiving a closing '
                'handshake')

        while True:
            
            
            
            frame_type_str = self.receive_bytes(1)
            frame_type = ord(frame_type_str)
            if (frame_type & 0x80) == 0x80:
                
                
                length = self._read_payload_length_hixie75()
                if length > 0:
                    _ = self.receive_bytes(length)
                
                
                if not self._enable_closing_handshake:
                    continue

                if frame_type == 0xFF and length == 0:
                    self._request.client_terminated = True

                    if self._request.server_terminated:
                        self._logger.debug(
                            'Received ack for server-initiated closing '
                            'handshake')
                        return None

                    self._logger.debug(
                        'Received client-initiated closing handshake')

                    self._send_closing_handshake()
                    self._logger.debug(
                        'Sent ack for client-initiated closing handshake')
                    return None
            else:
                
                bytes = self._read_until('\xff')
                
                
                
                message = bytes.decode('utf-8', 'replace')
                if frame_type == 0x00:
                    return message
                

    def _send_closing_handshake(self):
        if not self._enable_closing_handshake:
            raise BadOperationException(
                'Closing handshake is not supported in Hixie 75 protocol')

        self._request.server_terminated = True

        
        
        
        
        self._write('\xff\x00')

    def close_connection(self, unused_code='', unused_reason=''):
        """Closes a WebSocket connection.

        Raises:
            ConnectionTerminatedException: when closing handshake was
                not successfull.
        """

        if self._request.server_terminated:
            self._logger.debug(
                'Requested close_connection but server is already terminated')
            return

        if not self._enable_closing_handshake:
            self._request.server_terminated = True
            self._logger.debug('Connection closed')
            return

        self._send_closing_handshake()
        self._logger.debug('Sent server-initiated closing handshake')

        
        
        
        
        
        
        message = self.receive_message()
        if message is not None:
            raise ConnectionTerminatedException(
                'Didn\'t receive valid ack for closing handshake')
        
        

    def send_ping(self, body):
        raise BadOperationException(
            'StreamHixie75 doesn\'t support send_ping')



