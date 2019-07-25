





























"""Message related utilities.

Note: request.connection.write/read are used in this module, even though
mod_python document says that they should be used only in connection handlers.
Unfortunately, we have no other options. For example, request.write/read are
not suitable because they don't allow direct raw bytes writing/reading.
"""


import Queue
import threading

from mod_pywebsocket import util
from time import time,sleep


class MsgUtilException(Exception):
    pass


class ConnectionTerminatedException(MsgUtilException):
    pass


def _read(request, length):
    bytes = request.connection.read(length)
    if not bytes:
        raise MsgUtilException(
                'Failed to receive message from %r' %
                        (request.connection.remote_addr,))
    return bytes


def _write(request, bytes):
    try:
        request.connection.write(bytes)
    except Exception, e:
        util.prepend_message_to_exception(
                'Failed to send message to %r: ' %
                        (request.connection.remote_addr,),
                e)
        raise


def close_connection(request, abort=False):
    """Close connection.

    Args:
        request: mod_python request.
    """
    if request.server_terminated:
        return
    
    
    
    
    got_exception = False
    if not abort:
        _write(request, '\xff\x00')
        
        initial_time = time()
        end_time = initial_time + 20
        while time() < end_time:
            try:
                receive_message(request)
            except ConnectionTerminatedException, e:
                got_exception = True
                sleep(1)
    request.server_terminated = True
    if got_exception:
        util.prepend_message_to_exception(
            'client initiated closing handshake for %s: ' % (
            request.ws_resource),
            e)
        raise ConnectionTerminatedException
    
    


def send_message(request, message):
    """Send message.

    Args:
        request: mod_python request.
        message: unicode string to send.
    Raises:
        ConnectionTerminatedException: when server already terminated.
    """
    if request.server_terminated:
        raise ConnectionTerminatedException
    _write(request, '\x00' + message.encode('utf-8') + '\xff')


def receive_message(request):
    """Receive a Web Socket frame and return its payload as unicode string.

    Args:
        request: mod_python request.
    Raises:
        ConnectionTerminatedException: when client already terminated.
    """

    if request.client_terminated:
        raise ConnectionTerminatedException
    while True:
        
        
        
        frame_type_str = _read(request, 1)
        frame_type = ord(frame_type_str[0])
        if (frame_type & 0x80) == 0x80:
            
            
            length = _payload_length(request)
            _receive_bytes(request, length)
            
            
            if frame_type == 0xFF and length == 0:
                request.client_terminated = True
                raise ConnectionTerminatedException
        else:
            
            bytes = _read_until(request, '\xff')
            
            
            message = bytes.decode('utf-8', 'replace')
            if frame_type == 0x00:
                return message
            


def _payload_length(request):
    length = 0
    while True:
        b_str = _read(request, 1)
        b = ord(b_str[0])
        length = length * 128 + (b & 0x7f)
        if (b & 0x80) == 0:
            break
    return length


def _receive_bytes(request, length):
    bytes = []
    while length > 0:
        new_bytes = _read(request, length)
        bytes.append(new_bytes)
        length -= len(new_bytes)
    return ''.join(bytes)


def _read_until(request, delim_char):
    bytes = []
    while True:
        ch = _read(request, 1)
        if ch == delim_char:
            break
        bytes.append(ch)
    return ''.join(bytes)


class MessageReceiver(threading.Thread):
    """This class receives messages from the client.

    This class provides three ways to receive messages: blocking, non-blocking,
    and via callback. Callback has the highest precedence.

    Note: This class should not be used with the standalone server for wss
    because pyOpenSSL used by the server raises a fatal error if the socket
    is accessed from multiple threads.
    """
    def __init__(self, request, onmessage=None):
        """Construct an instance.

        Args:
            request: mod_python request.
            onmessage: a function to be called when a message is received.
                       May be None. If not None, the function is called on
                       another thread. In that case, MessageReceiver.receive
                       and MessageReceiver.receive_nowait are useless because
                       they will never return any messages.
        """
        threading.Thread.__init__(self)
        self._request = request
        self._queue = Queue.Queue()
        self._onmessage = onmessage
        self._stop_requested = False
        self.setDaemon(True)
        self.start()

    def run(self):
        try:
            while not self._stop_requested:
                message = receive_message(self._request)
                if self._onmessage:
                    self._onmessage(message)
                else:
                    self._queue.put(message)
        finally:
            close_connection(self._request)

    def receive(self):
        """ Receive a message from the channel, blocking.

        Returns:
            message as a unicode string.
        """
        return self._queue.get()

    def receive_nowait(self):
        """ Receive a message from the channel, non-blocking.

        Returns:
            message as a unicode string if available. None otherwise.
        """
        try:
            message = self._queue.get_nowait()
        except Queue.Empty:
            message = None
        return message

    def stop(self):
        """Request to stop this instance.

        The instance will be stopped after receiving the next message.
        This method may not be very useful, but there is no clean way
        in Python to forcefully stop a running thread.
        """
        self._stop_requested = True


class MessageSender(threading.Thread):
    """This class sends messages to the client.

    This class provides both synchronous and asynchronous ways to send
    messages.

    Note: This class should not be used with the standalone server for wss
    because pyOpenSSL used by the server raises a fatal error if the socket
    is accessed from multiple threads.
    """
    def __init__(self, request):
        """Construct an instance.

        Args:
            request: mod_python request.
        """
        threading.Thread.__init__(self)
        self._request = request
        self._queue = Queue.Queue()
        self.setDaemon(True)
        self.start()

    def run(self):
        while True:
            message, condition = self._queue.get()
            condition.acquire()
            send_message(self._request, message)
            condition.notify()
            condition.release()

    def send(self, message):
        """Send a message, blocking."""

        condition = threading.Condition()
        condition.acquire()
        self._queue.put((message, condition))
        condition.wait()

    def send_nowait(self, message):
        """Send a message, non-blocking."""

        self._queue.put((message, threading.Condition()))



