





























"""Base stream class.
"""









import socket

from mod_pywebsocket import util





class ConnectionTerminatedException(Exception):
    """This exception will be raised when a connection is terminated
    unexpectedly.
    """

    pass


class InvalidFrameException(ConnectionTerminatedException):
    """This exception will be raised when we received an invalid frame we
    cannot parse.
    """

    pass


class BadOperationException(Exception):
    """This exception will be raised when send_message() is called on
    server-terminated connection or receive_message() is called on
    client-terminated connection.
    """

    pass


class UnsupportedFrameException(Exception):
    """This exception will be raised when we receive a frame with flag, opcode
    we cannot handle. Handlers can just catch and ignore this exception and
    call receive_message() again to continue processing the next frame.
    """

    pass


class InvalidUTF8Exception(Exception):
    """This exception will be raised when we receive a text frame which
    contains invalid UTF-8 strings.
    """

    pass


class StreamBase(object):
    """Base stream class."""

    def __init__(self, request):
        """Construct an instance.

        Args:
            request: mod_python request.
        """

        self._logger = util.get_class_logger(self)

        self._request = request

    def _read(self, length):
        """Reads length bytes from connection. In case we catch any exception,
        prepends remote address to the exception message and raise again.

        Raises:
            ConnectionTerminatedException: when read returns empty string.
        """

        try:
            read_bytes = self._request.connection.read(length)
            if not read_bytes:
                raise ConnectionTerminatedException(
                    'Receiving %d byte failed. Peer (%r) closed connection' %
                    (length, (self._request.connection.remote_addr,)))
            return read_bytes
        except socket.error, e:
            
            
            
            
            raise ConnectionTerminatedException(
                'Receiving %d byte failed. socket.error (%s) occurred' %
                (length, e))
        except IOError, e:
            
            raise ConnectionTerminatedException(
                'Receiving %d byte failed. IOError (%s) occurred' %
                (length, e))

    def _write(self, bytes_to_write):
        """Writes given bytes to connection. In case we catch any exception,
        prepends remote address to the exception message and raise again.
        """

        try:
            self._request.connection.write(bytes_to_write)
        except Exception, e:
            util.prepend_message_to_exception(
                    'Failed to send message to %r: ' %
                            (self._request.connection.remote_addr,),
                    e)
            raise

    def receive_bytes(self, length):
        """Receives multiple bytes. Retries read when we couldn't receive the
        specified amount.

        Raises:
            ConnectionTerminatedException: when read returns empty string.
        """

        read_bytes = []
        while length > 0:
            new_read_bytes = self._read(length)
            read_bytes.append(new_read_bytes)
            length -= len(new_read_bytes)
        return ''.join(read_bytes)

    def _read_until(self, delim_char):
        """Reads bytes until we encounter delim_char. The result will not
        contain delim_char.

        Raises:
            ConnectionTerminatedException: when read returns empty string.
        """

        read_bytes = []
        while True:
            ch = self._read(1)
            if ch == delim_char:
                break
            read_bytes.append(ch)
        return ''.join(read_bytes)



