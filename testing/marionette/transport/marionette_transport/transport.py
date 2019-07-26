



import errno
import json
import socket


class MarionetteTransport(object):
    """ The Marionette socket client.  This speaks the same protocol
        as the remote debugger inside Gecko, in which messages are
        always preceded by the message length and a colon, e.g.,

        20:{'command': 'test'}
    """

    max_packet_length = 4096
    connection_lost_msg = "Connection to Marionette server is lost. Check gecko.log (desktop firefox) or logcat (b2g) for errors."

    def __init__(self, addr, port):
        self.addr = addr
        self.port = port
        self.sock = None
        self.traits = None
        self.applicationType = None
        self.actor = 'root'

    def _recv_n_bytes(self, n):
        """ Convenience method for receiving exactly n bytes from
            self.sock (assuming it's open and connected).
        """
        data = ''
        while len(data) < n:
            chunk = self.sock.recv(n - len(data))
            if chunk == '':
                break
            data += chunk
        return data

    def receive(self):
        """ Receive the next complete response from the server, and return
            it as a dict.  Each response from the server is prepended by
            len(message) + ':'.
        """
        assert(self.sock)
        response = self.sock.recv(10)
        sep = response.find(':')
        length = response[0:sep]
        if length != '':
            response = response[sep + 1:]
            response += self._recv_n_bytes(int(length) + 1 + len(length) - 10)
            return json.loads(response)
        else:
            raise IOError(self.connection_lost_msg)

    def connect(self, timeout=360.0):
        """ Connect to the server and process the hello message we expect
            to receive in response.
        """
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(timeout)
        try:
            self.sock.connect((self.addr, self.port))
        except:
            
            
            self.sock = None
            raise
        hello = self.receive()
        self.traits = hello.get('traits')
        self.applicationType = hello.get('applicationType')

        
        response = self.send({'to': 'root', 'name': 'getMarionetteID'})
        self.actor = response['id']

    def send(self, msg):
        """ Send a message on the socket, prepending it with len(msg) + ':'.
        """
        if not self.sock:
            self.connect()
        if 'to' not in msg:
            msg['to'] = self.actor
        data = json.dumps(msg)
        data = '%s:%s' % (len(data), data)

        for packet in [data[i:i + self.max_packet_length] for i in
                       range(0, len(data), self.max_packet_length)]:
            try: 
                self.sock.send(packet)
            except IOError as e:
                if e.errno == errno.EPIPE:
                    raise IOError("%s: %s" % (str(e)), self.connection_lost_msg)
                else:
                    raise e

        response = self.receive()
        return response

    def close(self):
        """ Close the socket.
        """
        self.sock.close()
        self.sock = None
