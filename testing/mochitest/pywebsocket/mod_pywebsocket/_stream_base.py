





























"""Base stream class.
"""









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


class BadOperationException(RuntimeError):
    """This exception will be raised when send_message() is called on
    server-terminated connection or receive_message() is called on
    client-terminated connection.
    """
    pass


class UnsupportedFrameException(RuntimeError):
    """This exception will be raised when we receive a frame with flag, opcode
    we cannot handle. Handlers can just catch and ignore this exception and
    call receive_message() again to continue processing the next frame.
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

        bytes = self._request.connection.read(length)
        if not bytes:
            raise ConnectionTerminatedException('connection terminated: read failed')
        return bytes

    def _write(self, bytes):
        """Writes given bytes to connection. In case we catch any exception,
        prepends remote address to the exception message and raise again.
        """

        try:
            self._request.connection.write(bytes)
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

        bytes = []
        while length > 0:
            new_bytes = self._read(length)
            bytes.append(new_bytes)
            length -= len(new_bytes)
        return ''.join(bytes)

    def _read_until(self, delim_char):
        """Reads bytes until we encounter delim_char. The result will not
        contain delim_char.

        Raises:
            ConnectionTerminatedException: when read returns empty string.
        """

        bytes = []
        while True:
            ch = self._read(1)
            if ch == delim_char:
                break
            bytes.append(ch)
        return ''.join(bytes)



