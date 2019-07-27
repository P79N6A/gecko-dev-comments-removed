































"""Simple WebSocket client named echo_client just because of historical reason.

mod_pywebsocket directory must be in PYTHONPATH.

Example Usage:

# server setup
 % cd $pywebsocket
 % PYTHONPATH=$cwd/src python ./mod_pywebsocket/standalone.py -p 8880 \
    -d $cwd/src/example

# run client
 % PYTHONPATH=$cwd/src python ./src/example/echo_client.py -p 8880 \
     -s localhost \
     -o http://localhost -r /echo -m test

or

# run echo client to test IETF HyBi 00 protocol
 run with --protocol-version=hybi00
"""


import base64
import codecs
import logging
from optparse import OptionParser
import os
import random
import re
import socket
import struct
import sys

from mod_pywebsocket import common
from mod_pywebsocket.extensions import DeflateFrameExtensionProcessor
from mod_pywebsocket.extensions import PerMessageDeflateExtensionProcessor
from mod_pywebsocket.extensions import _PerMessageDeflateFramer
from mod_pywebsocket.extensions import _parse_window_bits
from mod_pywebsocket.stream import Stream
from mod_pywebsocket.stream import StreamHixie75
from mod_pywebsocket.stream import StreamOptions
from mod_pywebsocket import util


_TIMEOUT_SEC = 10
_UNDEFINED_PORT = -1

_UPGRADE_HEADER = 'Upgrade: websocket\r\n'
_UPGRADE_HEADER_HIXIE75 = 'Upgrade: WebSocket\r\n'
_CONNECTION_HEADER = 'Connection: Upgrade\r\n'


_GOODBYE_MESSAGE = 'Goodbye'

_PROTOCOL_VERSION_HYBI13 = 'hybi13'
_PROTOCOL_VERSION_HYBI08 = 'hybi08'
_PROTOCOL_VERSION_HYBI00 = 'hybi00'
_PROTOCOL_VERSION_HIXIE75 = 'hixie75'


_TLS_BY_STANDARD_MODULE = 'ssl'
_TLS_BY_PYOPENSSL = 'pyopenssl'


_TLS_VERSION_SSL23 = 'ssl23'
_TLS_VERSION_SSL3 = 'ssl3'
_TLS_VERSION_TLS1 = 'tls1'


class ClientHandshakeError(Exception):
    pass


def _build_method_line(resource):
    return 'GET %s HTTP/1.1\r\n' % resource


def _origin_header(header, origin):
    
    
    return '%s: %s\r\n' % (header, origin.lower())


def _format_host_header(host, port, secure):
    
    
    
    hostport = host.lower()
    
    
    
    
    if ((not secure and port != common.DEFAULT_WEB_SOCKET_PORT) or
        (secure and port != common.DEFAULT_WEB_SOCKET_SECURE_PORT)):
        hostport += ':' + str(port)
    
    
    return '%s: %s\r\n' % (common.HOST_HEADER, hostport)


def _receive_bytes(socket, length):
    bytes = []
    remaining = length
    while remaining > 0:
        received_bytes = socket.recv(remaining)
        if not received_bytes:
            raise IOError(
                'Connection closed before receiving requested length '
                '(requested %d bytes but received only %d bytes)' %
                (length, length - remaining))
        bytes.append(received_bytes)
        remaining -= len(received_bytes)
    return ''.join(bytes)


def _get_mandatory_header(fields, name):
    """Gets the value of the header specified by name from fields.

    This function expects that there's only one header with the specified name
    in fields. Otherwise, raises an ClientHandshakeError.
    """

    values = fields.get(name.lower())
    if values is None or len(values) == 0:
        raise ClientHandshakeError(
            '%s header not found: %r' % (name, values))
    if len(values) > 1:
        raise ClientHandshakeError(
            'Multiple %s headers found: %r' % (name, values))
    return values[0]


def _validate_mandatory_header(fields, name,
                               expected_value, case_sensitive=False):
    """Gets and validates the value of the header specified by name from
    fields.

    If expected_value is specified, compares expected value and actual value
    and raises an ClientHandshakeError on failure. You can specify case
    sensitiveness in this comparison by case_sensitive parameter. This function
    expects that there's only one header with the specified name in fields.
    Otherwise, raises an ClientHandshakeError.
    """

    value = _get_mandatory_header(fields, name)

    if ((case_sensitive and value != expected_value) or
        (not case_sensitive and value.lower() != expected_value.lower())):
        raise ClientHandshakeError(
            'Illegal value for header %s: %r (expected) vs %r (actual)' %
            (name, expected_value, value))


class _TLSSocket(object):
    """Wrapper for a TLS connection."""

    def __init__(self,
                 raw_socket, tls_module, tls_version, disable_tls_compression):
        self._logger = util.get_class_logger(self)

        if tls_module == _TLS_BY_STANDARD_MODULE:
            if tls_version == _TLS_VERSION_SSL23:
                version = ssl.PROTOCOL_SSLv23
            elif tls_version == _TLS_VERSION_SSL3:
                version = ssl.PROTOCOL_SSLv3
            elif tls_version == _TLS_VERSION_TLS1:
                version = ssl.PROTOCOL_TLSv1
            else:
                raise ValueError(
                    'Invalid --tls-version flag: %r' % tls_version)

            if disable_tls_compression:
                raise ValueError(
                    '--disable-tls-compression is not available for ssl '
                    'module')

            self._tls_socket = ssl.wrap_socket(raw_socket, ssl_version=version)

            
            self._logger.info("Cipher: %s", self._tls_socket.cipher())
        elif tls_module == _TLS_BY_PYOPENSSL:
            if tls_version == _TLS_VERSION_SSL23:
                version = OpenSSL.SSL.SSLv23_METHOD
            elif tls_version == _TLS_VERSION_SSL3:
                version = OpenSSL.SSL.SSLv3_METHOD
            elif tls_version == _TLS_VERSION_TLS1:
                version = OpenSSL.SSL.TLSv1_METHOD
            else:
                raise ValueError(
                    'Invalid --tls-version flag: %r' % tls_version)

            context = OpenSSL.SSL.Context(version)

            if disable_tls_compression:
                
                context.set_options(0x00020000)

            self._tls_socket = OpenSSL.SSL.Connection(context, raw_socket)
            
            self._tls_socket.set_connect_state()
            self._tls_socket.setblocking(True)

            
            self._tls_socket.do_handshake()
        else:
            raise ValueError('No TLS support module is available')

    def send(self, data):
        return self._tls_socket.write(data)

    def sendall(self, data):
        return self._tls_socket.sendall(data)

    def recv(self, size=-1):
        return self._tls_socket.read(size)

    def close(self):
        return self._tls_socket.close()

    def getpeername(self):
        return self._tls_socket.getpeername()


class ClientHandshakeBase(object):
    """A base class for WebSocket opening handshake processors for each
    protocol version.
    """

    def __init__(self):
        self._logger = util.get_class_logger(self)

    def _read_fields(self):
        
        fields = {}
        while True:  
            
            name = ''
            value = ''
            
            name = self._read_name()
            if name is None:
                break
            
            
            ch = self._skip_spaces()
            
            value = self._read_value(ch)
            
            ch = _receive_bytes(self._socket, 1)
            if ch != '\n':  
                raise ClientHandshakeError(
                    'Expected LF but found %r while reading value %r for '
                    'header %r' % (ch, value, name))
            self._logger.debug('Received %r header', name)
            
            
            
            
            
            fields.setdefault(name, []).append(value)
            
        return fields

    def _read_name(self):
        
        name = ''
        while True:
            
            ch = _receive_bytes(self._socket, 1)
            if ch == '\r':  
                return None
            elif ch == '\n':  
                raise ClientHandshakeError(
                    'Unexpected LF when reading header name %r' % name)
            elif ch == ':':  
                return name
            elif ch >= 'A' and ch <= 'Z':  
                ch = chr(ord(ch) + 0x20)
                name += ch
            else:
                name += ch

    def _skip_spaces(self):
        
        while True:
            ch = _receive_bytes(self._socket, 1)
            if ch == ' ':  
                continue
            return ch

    def _read_value(self, ch):
        
        value = ''
        
        while True:
            if ch == '\r':  
                return value
            elif ch == '\n':  
                raise ClientHandshakeError(
                    'Unexpected LF when reading header value %r' % value)
            else:
                value += ch
            ch = _receive_bytes(self._socket, 1)


def _get_permessage_deflate_framer(extension_response):
    """Validate the response and return a framer object using the parameters in
    the response. This method doesn't accept the s2c_.* parameters.
    """

    c2s_max_window_bits = None
    c2s_no_context_takeover = None

    c2s_max_window_bits_name = (
            PerMessageDeflateExtensionProcessor.
                    _C2S_MAX_WINDOW_BITS_PARAM)
    c2s_no_context_takeover_name = (
            PerMessageDeflateExtensionProcessor.
                    _C2S_NO_CONTEXT_TAKEOVER_PARAM)

    
    

    for param_name, param_value in extension_response.get_parameters():
        if param_name == c2s_max_window_bits_name:
            if c2s_max_window_bits is not None:
                raise ClientHandshakeError(
                        'Multiple %s found' % c2s_max_window_bits_name)

            parsed_value = _parse_window_bits(param_value)
            if parsed_value is None:
                raise ClientHandshakeError(
                        'Bad %s: %r' %
                        (c2s_max_window_bits_name, param_value))
            c2s_max_window_bits = parsed_value
        elif param_name == c2s_no_context_takeover_name:
            if c2s_no_context_takeover is not None:
                raise ClientHandshakeError(
                        'Multiple %s found' % c2s_no_context_takeover_name)

            if param_value is not None:
                raise ClientHandshakeError(
                        'Bad %s: Has value %r' %
                        (c2s_no_context_takeover_name, param_value))
            c2s_no_context_takeover = True

    if c2s_no_context_takeover is None:
        c2s_no_context_takeover = False

    return _PerMessageDeflateFramer(c2s_max_window_bits,
                                    c2s_no_context_takeover)


class ClientHandshakeProcessor(ClientHandshakeBase):
    """WebSocket opening handshake processor for
    draft-ietf-hybi-thewebsocketprotocol-06 and later.
    """

    def __init__(self, socket, options):
        super(ClientHandshakeProcessor, self).__init__()

        self._socket = socket
        self._options = options

        self._logger = util.get_class_logger(self)

    def handshake(self):
        """Performs opening handshake on the specified socket.

        Raises:
            ClientHandshakeError: handshake failed.
        """

        request_line = _build_method_line(self._options.resource)
        self._logger.debug('Client\'s opening handshake Request-Line: %r',
                           request_line)
        self._socket.sendall(request_line)

        fields = []
        fields.append(_format_host_header(
            self._options.server_host,
            self._options.server_port,
            self._options.use_tls))
        fields.append(_UPGRADE_HEADER)
        fields.append(_CONNECTION_HEADER)
        if self._options.origin is not None:
            if self._options.protocol_version == _PROTOCOL_VERSION_HYBI08:
                fields.append(_origin_header(
                    common.SEC_WEBSOCKET_ORIGIN_HEADER,
                    self._options.origin))
            else:
                fields.append(_origin_header(common.ORIGIN_HEADER,
                                             self._options.origin))

        original_key = os.urandom(16)
        self._key = base64.b64encode(original_key)
        self._logger.debug(
            '%s: %r (%s)',
            common.SEC_WEBSOCKET_KEY_HEADER,
            self._key,
            util.hexify(original_key))
        fields.append(
            '%s: %s\r\n' % (common.SEC_WEBSOCKET_KEY_HEADER, self._key))

        if self._options.version_header > 0:
            fields.append('%s: %d\r\n' % (common.SEC_WEBSOCKET_VERSION_HEADER,
                                          self._options.version_header))
        elif self._options.protocol_version == _PROTOCOL_VERSION_HYBI08:
            fields.append('%s: %d\r\n' % (common.SEC_WEBSOCKET_VERSION_HEADER,
                                          common.VERSION_HYBI08))
        else:
            fields.append('%s: %d\r\n' % (common.SEC_WEBSOCKET_VERSION_HEADER,
                                          common.VERSION_HYBI_LATEST))

        extensions_to_request = []

        if self._options.deflate_frame:
            extensions_to_request.append(
                common.ExtensionParameter(common.DEFLATE_FRAME_EXTENSION))

        if self._options.use_permessage_deflate:
            extension = common.ExtensionParameter(
                    common.PERMESSAGE_DEFLATE_EXTENSION)
            
            extension.add_parameter(
                    PerMessageDeflateExtensionProcessor.
                            _C2S_MAX_WINDOW_BITS_PARAM,
                    None)
            extensions_to_request.append(extension)

        if len(extensions_to_request) != 0:
            fields.append(
                '%s: %s\r\n' %
                (common.SEC_WEBSOCKET_EXTENSIONS_HEADER,
                 common.format_extensions(extensions_to_request)))

        for field in fields:
            self._socket.sendall(field)

        self._socket.sendall('\r\n')

        self._logger.debug('Sent client\'s opening handshake headers: %r',
                           fields)
        self._logger.debug('Start reading Status-Line')

        status_line = ''
        while True:
            ch = _receive_bytes(self._socket, 1)
            status_line += ch
            if ch == '\n':
                break

        m = re.match('HTTP/\\d+\.\\d+ (\\d\\d\\d) .*\r\n', status_line)
        if m is None:
            raise ClientHandshakeError(
                'Wrong status line format: %r' % status_line)
        status_code = m.group(1)
        if status_code != '101':
            self._logger.debug('Unexpected status code %s with following '
                               'headers: %r', status_code, self._read_fields())
            raise ClientHandshakeError(
                'Expected HTTP status code 101 but found %r' % status_code)

        self._logger.debug('Received valid Status-Line')
        self._logger.debug('Start reading headers until we see an empty line')

        fields = self._read_fields()

        ch = _receive_bytes(self._socket, 1)
        if ch != '\n':  
            raise ClientHandshakeError(
                'Expected LF but found %r while reading value %r for header '
                'name %r' % (ch, value, name))

        self._logger.debug('Received an empty line')
        self._logger.debug('Server\'s opening handshake headers: %r', fields)

        _validate_mandatory_header(
            fields,
            common.UPGRADE_HEADER,
            common.WEBSOCKET_UPGRADE_TYPE,
            False)

        _validate_mandatory_header(
            fields,
            common.CONNECTION_HEADER,
            common.UPGRADE_CONNECTION_TYPE,
            False)

        accept = _get_mandatory_header(
            fields, common.SEC_WEBSOCKET_ACCEPT_HEADER)

        
        try:
            binary_accept = base64.b64decode(accept)
        except TypeError, e:
            raise HandshakeError(
                'Illegal value for header %s: %r' %
                (common.SEC_WEBSOCKET_ACCEPT_HEADER, accept))

        if len(binary_accept) != 20:
            raise ClientHandshakeError(
                'Decoded value of %s is not 20-byte long' %
                common.SEC_WEBSOCKET_ACCEPT_HEADER)

        self._logger.debug(
            'Response for challenge : %r (%s)',
            accept, util.hexify(binary_accept))

        binary_expected_accept = util.sha1_hash(
            self._key + common.WEBSOCKET_ACCEPT_UUID).digest()
        expected_accept = base64.b64encode(binary_expected_accept)

        self._logger.debug(
            'Expected response for challenge: %r (%s)',
            expected_accept, util.hexify(binary_expected_accept))

        if accept != expected_accept:
            raise ClientHandshakeError(
                'Invalid %s header: %r (expected: %s)' %
                (common.SEC_WEBSOCKET_ACCEPT_HEADER, accept, expected_accept))

        deflate_frame_accepted = False
        permessage_deflate_accepted = False

        extensions_header = fields.get(
            common.SEC_WEBSOCKET_EXTENSIONS_HEADER.lower())
        accepted_extensions = []
        if extensions_header is not None and len(extensions_header) != 0:
            accepted_extensions = common.parse_extensions(extensions_header[0])

        
        for extension in accepted_extensions:
            extension_name = extension.name()
            if (extension_name == common.DEFLATE_FRAME_EXTENSION and
                self._options.deflate_frame):
                deflate_frame_accepted = True
                processor = DeflateFrameExtensionProcessor(extension)
                unused_extension_response = processor.get_extension_response()
                self._options.deflate_frame = processor
                continue
            elif (extension_name == common.PERMESSAGE_DEFLATE_EXTENSION and
                  self._options.use_permessage_deflate):
                permessage_deflate_accepted = True

                framer = _get_permessage_deflate_framer(extension)
                framer.set_compress_outgoing_enabled(True)
                self._options.use_permessage_deflate = framer
                continue

            raise ClientHandshakeError(
                'Unexpected extension %r' % extension_name)

        if (self._options.deflate_frame and not deflate_frame_accepted):
            raise ClientHandshakeError(
                'Requested %s, but the server rejected it' %
                common.DEFLATE_FRAME_EXTENSION)

        if (self._options.use_permessage_deflate and
            not permessage_deflate_accepted):
            raise ClientHandshakeError(
                    'Requested %s, but the server rejected it' %
                    common.PERMESSAGE_DEFLATE_EXTENSION)

        
        


class ClientHandshakeProcessorHybi00(ClientHandshakeBase):
    """WebSocket opening handshake processor for
    draft-ietf-hybi-thewebsocketprotocol-00 (equivalent to
    draft-hixie-thewebsocketprotocol-76).
    """

    def __init__(self, socket, options):
        super(ClientHandshakeProcessorHybi00, self).__init__()

        self._socket = socket
        self._options = options

        self._logger = util.get_class_logger(self)

        if (self._options.deflate_frame or
            self._options.use_permessage_deflate):
            logging.critical('HyBi 00 doesn\'t support extensions.')
            sys.exit(1)

    def handshake(self):
        """Performs opening handshake on the specified socket.

        Raises:
            ClientHandshakeError: handshake failed.
        """

        
        self._socket.sendall(_build_method_line(self._options.resource))
        
        fields = []
        
        fields.append(_UPGRADE_HEADER_HIXIE75)
        
        fields.append(_CONNECTION_HEADER)
        
        fields.append(_format_host_header(
            self._options.server_host,
            self._options.server_port,
            self._options.use_tls))
        
        if not self._options.origin:
            raise ClientHandshakeError(
                'Specify the origin of the connection by --origin flag')
        fields.append(_origin_header(common.ORIGIN_HEADER,
                                     self._options.origin))
        
        

        
        self._number1, key1 = self._generate_sec_websocket_key()
        self._logger.debug('Number1: %d', self._number1)
        fields.append('%s: %s\r\n' % (common.SEC_WEBSOCKET_KEY1_HEADER, key1))
        self._number2, key2 = self._generate_sec_websocket_key()
        self._logger.debug('Number2: %d', self._number2)
        fields.append('%s: %s\r\n' % (common.SEC_WEBSOCKET_KEY2_HEADER, key2))

        fields.append('%s: 0\r\n' % common.SEC_WEBSOCKET_DRAFT_HEADER)

        
        
        
        random.shuffle(fields)
        for field in fields:
            self._socket.sendall(field)
        
        
        self._socket.sendall('\r\n')
        
        
        self._key3 = self._generate_key3()
        
        self._socket.sendall(self._key3)
        self._logger.debug(
            'Key3: %r (%s)', self._key3, util.hexify(self._key3))

        self._logger.info('Sent handshake')

        
        
        
        field = ''
        while True:
            ch = _receive_bytes(self._socket, 1)
            field += ch
            if ch == '\n':
                break
        
        
        
        
        if len(field) < 7 or not field.endswith('\r\n'):
            raise ClientHandshakeError('Wrong status line: %r' % field)
        m = re.match('[^ ]* ([^ ]*) .*', field)
        if m is None:
            raise ClientHandshakeError(
                'No HTTP status code found in status line: %r' % field)
        
        
        
        code = m.group(1)
        
        
        
        if not re.match('[0-9][0-9][0-9]', code):
            raise ClientHandshakeError(
                'HTTP status code %r is not three digit in status line: %r' %
                (code, field))
        
        
        if code != '101':
            raise ClientHandshakeError(
                'Expected HTTP status code 101 but found %r in status line: '
                '%r' % (code, field))
        
        fields = self._read_fields()
        
        
        ch = _receive_bytes(self._socket, 1)
        if ch != '\n':  
            raise ClientHandshakeError('Expected LF but found %r' % ch)
        
        
        
        
        
        _validate_mandatory_header(
            fields,
            common.UPGRADE_HEADER,
            common.WEBSOCKET_UPGRADE_TYPE_HIXIE75,
            True)
        
        
        
        
        _validate_mandatory_header(
            fields,
            common.CONNECTION_HEADER,
            common.UPGRADE_CONNECTION_TYPE,
            False)

        origin = _get_mandatory_header(
            fields, common.SEC_WEBSOCKET_ORIGIN_HEADER)

        location = _get_mandatory_header(
            fields, common.SEC_WEBSOCKET_LOCATION_HEADER)

        

        
        
        
        
        challenge = struct.pack('!I', self._number1)
        challenge += struct.pack('!I', self._number2)
        challenge += self._key3

        self._logger.debug(
            'Challenge: %r (%s)', challenge, util.hexify(challenge))

        
        
        expected = util.md5_hash(challenge).digest()
        self._logger.debug(
            'Expected challenge response: %r (%s)',
            expected, util.hexify(expected))

        
        
        reply = _receive_bytes(self._socket, 16)
        self._logger.debug(
            'Actual challenge response: %r (%s)', reply, util.hexify(reply))

        
        
        if expected != reply:
            raise ClientHandshakeError(
                'Bad challenge response: %r (expected) != %r (actual)' %
                (expected, reply))
        

    def _generate_sec_websocket_key(self):
        
        spaces = random.randint(1, 12)
        
        
        maxnum = 4294967295 / spaces
        
        
        number = random.randint(0, maxnum)
        
        
        product = number * spaces
        
        
        
        key = str(product)
        
        
        
        available_chars = range(0x21, 0x2f + 1) + range(0x3a, 0x7e + 1)
        n = random.randint(1, 12)
        for _ in xrange(n):
            ch = random.choice(available_chars)
            pos = random.randint(0, len(key))
            key = key[0:pos] + chr(ch) + key[pos:]
        
        
        for _ in xrange(spaces):
            pos = random.randint(1, len(key) - 1)
            key = key[0:pos] + ' ' + key[pos:]
        return number, key

    def _generate_key3(self):
        
        
        return ''.join([chr(random.randint(0, 255)) for _ in xrange(8)])


class ClientConnection(object):
    """A wrapper for socket object to provide the mp_conn interface.
    mod_pywebsocket library is designed to be working on Apache mod_python's
    mp_conn object.
    """

    def __init__(self, socket):
        self._socket = socket

    def write(self, data):
        self._socket.sendall(data)

    def read(self, n):
        return self._socket.recv(n)

    def get_remote_addr(self):
        return self._socket.getpeername()
    remote_addr = property(get_remote_addr)


class ClientRequest(object):
    """A wrapper class just to make it able to pass a socket object to
    functions that expect a mp_request object.
    """

    def __init__(self, socket):
        self._logger = util.get_class_logger(self)

        self._socket = socket
        self.connection = ClientConnection(socket)


def _import_ssl():
    global ssl
    try:
        import ssl
        return True
    except ImportError:
        return False


def _import_pyopenssl():
    global OpenSSL
    try:
        import OpenSSL.SSL
        return True
    except ImportError:
        return False


class EchoClient(object):
    """WebSocket echo client."""

    def __init__(self, options):
        self._options = options
        self._socket = None

        self._logger = util.get_class_logger(self)

    def run(self):
        """Run the client.

        Shake hands and then repeat sending message and receiving its echo.
        """

        self._socket = socket.socket()
        self._socket.settimeout(self._options.socket_timeout)
        try:
            self._socket.connect((self._options.server_host,
                                  self._options.server_port))
            if self._options.use_tls:
                self._socket = _TLSSocket(
                    self._socket,
                    self._options.tls_module,
                    self._options.tls_version,
                    self._options.disable_tls_compression)

            version = self._options.protocol_version

            if (version == _PROTOCOL_VERSION_HYBI08 or
                version == _PROTOCOL_VERSION_HYBI13):
                self._handshake = ClientHandshakeProcessor(
                    self._socket, self._options)
            elif version == _PROTOCOL_VERSION_HYBI00:
                self._handshake = ClientHandshakeProcessorHybi00(
                    self._socket, self._options)
            else:
                raise ValueError(
                    'Invalid --protocol-version flag: %r' % version)

            self._handshake.handshake()

            self._logger.info('Connection established')

            request = ClientRequest(self._socket)

            version_map = {
                _PROTOCOL_VERSION_HYBI08: common.VERSION_HYBI08,
                _PROTOCOL_VERSION_HYBI13: common.VERSION_HYBI13,
                _PROTOCOL_VERSION_HYBI00: common.VERSION_HYBI00}
            request.ws_version = version_map[version]

            if (version == _PROTOCOL_VERSION_HYBI08 or
                version == _PROTOCOL_VERSION_HYBI13):
                stream_option = StreamOptions()
                stream_option.mask_send = True
                stream_option.unmask_receive = False

                if self._options.deflate_frame is not False:
                    processor = self._options.deflate_frame
                    processor.setup_stream_options(stream_option)

                if self._options.use_permessage_deflate is not False:
                    framer = self._options.use_permessage_deflate
                    framer.setup_stream_options(stream_option)

                self._stream = Stream(request, stream_option)
            elif version == _PROTOCOL_VERSION_HYBI00:
                self._stream = StreamHixie75(request, True)

            for line in self._options.message.split(','):
                self._stream.send_message(line)
                if self._options.verbose:
                    print 'Send: %s' % line
                try:
                    received = self._stream.receive_message()

                    if self._options.verbose:
                        print 'Recv: %s' % received
                except Exception, e:
                    if self._options.verbose:
                        print 'Error: %s' % e
                    raise

            self._do_closing_handshake()
        finally:
            self._socket.close()

    def _do_closing_handshake(self):
        """Perform closing handshake using the specified closing frame."""

        if self._options.message.split(',')[-1] == _GOODBYE_MESSAGE:
            
            
            self._logger.info('Wait for server-initiated closing handshake')
            message = self._stream.receive_message()
            if message is None:
                print 'Recv close'
                print 'Send ack'
                self._logger.info(
                    'Received closing handshake and sent ack')
                return
        print 'Send close'
        self._stream.close_connection()
        self._logger.info('Sent closing handshake')
        print 'Recv ack'
        self._logger.info('Received ack')


def main():
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout)

    parser = OptionParser()
    
    
    parser.add_option('-s', '--server-host', '--server_host',
                      dest='server_host', type='string',
                      default='localhost', help='server host')
    parser.add_option('-p', '--server-port', '--server_port',
                      dest='server_port', type='int',
                      default=_UNDEFINED_PORT, help='server port')
    parser.add_option('-o', '--origin', dest='origin', type='string',
                      default=None, help='origin')
    parser.add_option('-r', '--resource', dest='resource', type='string',
                      default='/echo', help='resource path')
    parser.add_option('-m', '--message', dest='message', type='string',
                      help=('comma-separated messages to send. '
                           '%s will force close the connection from server.' %
                            _GOODBYE_MESSAGE))
    parser.add_option('-q', '--quiet', dest='verbose', action='store_false',
                      default=True, help='suppress messages')
    parser.add_option('-t', '--tls', dest='use_tls', action='store_true',
                      default=False, help='use TLS (wss://). By default, '
                      'it looks for ssl and pyOpenSSL module and uses found '
                      'one. Use --tls-module option to specify which module '
                      'to use')
    parser.add_option('--tls-module', '--tls_module', dest='tls_module',
                      type='choice',
                      choices = [_TLS_BY_STANDARD_MODULE, _TLS_BY_PYOPENSSL],
                      help='Use ssl module if "%s" is specified. '
                      'Use pyOpenSSL module if "%s" is specified' %
                      (_TLS_BY_STANDARD_MODULE, _TLS_BY_PYOPENSSL))
    parser.add_option('--tls-version', '--tls_version',
                      dest='tls_version',
                      type='string', default=_TLS_VERSION_SSL23,
                      help='TLS/SSL version to use. One of \'' +
                      _TLS_VERSION_SSL23 + '\' (SSL version 2 or 3), \'' +
                      _TLS_VERSION_SSL3 + '\' (SSL version 3), \'' +
                      _TLS_VERSION_TLS1 + '\' (TLS version 1)')
    parser.add_option('--disable-tls-compression', '--disable_tls_compression',
                      dest='disable_tls_compression',
                      action='store_true', default=False,
                      help='Disable TLS compression. Available only when '
                      'pyOpenSSL module is used.')
    parser.add_option('-k', '--socket-timeout', '--socket_timeout',
                      dest='socket_timeout', type='int', default=_TIMEOUT_SEC,
                      help='Timeout(sec) for sockets')
    parser.add_option('--draft75', dest='draft75',
                      action='store_true', default=False,
                      help='Obsolete option. Don\'t use this.')
    parser.add_option('--protocol-version', '--protocol_version',
                      dest='protocol_version',
                      type='string', default=_PROTOCOL_VERSION_HYBI13,
                      help='WebSocket protocol version to use. One of \'' +
                      _PROTOCOL_VERSION_HYBI13 + '\', \'' +
                      _PROTOCOL_VERSION_HYBI08 + '\', \'' +
                      _PROTOCOL_VERSION_HYBI00 + '\'')
    parser.add_option('--version-header', '--version_header',
                      dest='version_header',
                      type='int', default=-1,
                      help='Specify Sec-WebSocket-Version header value')
    parser.add_option('--deflate-frame', '--deflate_frame',
                      dest='deflate_frame',
                      action='store_true', default=False,
                      help='Use the deflate-frame extension.')
    parser.add_option('--use-permessage-deflate', '--use_permessage_deflate',
                      dest='use_permessage_deflate',
                      action='store_true', default=False,
                      help='Use the permessage-deflate extension.')
    parser.add_option('--log-level', '--log_level', type='choice',
                      dest='log_level', default='warn',
                      choices=['debug', 'info', 'warn', 'error', 'critical'],
                      help='Log level.')

    (options, unused_args) = parser.parse_args()

    logging.basicConfig(level=logging.getLevelName(options.log_level.upper()))

    if options.draft75:
        logging.critical('--draft75 option is obsolete.')
        sys.exit(1)

    if options.protocol_version == _PROTOCOL_VERSION_HIXIE75:
        logging.critical(
            'Value %s is obsolete for --protocol_version options' %
            _PROTOCOL_VERSION_HIXIE75)
        sys.exit(1)

    if options.use_tls:
        if options.tls_module is None:
            if _import_ssl():
                options.tls_module = _TLS_BY_STANDARD_MODULE
                logging.debug('Using ssl module')
            elif _import_pyopenssl():
                options.tls_module = _TLS_BY_PYOPENSSL
                logging.debug('Using pyOpenSSL module')
            else:
                logging.critical(
                        'TLS support requires ssl or pyOpenSSL module.')
                sys.exit(1)
        elif options.tls_module == _TLS_BY_STANDARD_MODULE:
            if not _import_ssl():
                logging.critical('ssl module is not available')
                sys.exit(1)
        elif options.tls_module == _TLS_BY_PYOPENSSL:
            if not _import_pyopenssl():
                logging.critical('pyOpenSSL module is not available')
                sys.exit(1)
        else:
            logging.critical('Invalid --tls-module option: %r',
                             options.tls_module)
            sys.exit(1)

        if (options.disable_tls_compression and
            options.tls_module != _TLS_BY_PYOPENSSL):
            logging.critical('You can disable TLS compression only when '
                             'pyOpenSSL module is used.')
            sys.exit(1)
    else:
        if options.tls_module is not None:
            logging.critical('Use --tls-module option only together with '
                             '--use-tls option.')
            sys.exit(1)

        if options.disable_tls_compression:
            logging.critical('Use --disable-tls-compression only together '
                             'with --use-tls option.')
            sys.exit(1)

    
    if options.server_port == _UNDEFINED_PORT:
        if options.use_tls:
            options.server_port = common.DEFAULT_WEB_SOCKET_SECURE_PORT
        else:
            options.server_port = common.DEFAULT_WEB_SOCKET_PORT

    
    
    if not options.message:
        options.message = u'Hello,\u65e5\u672c'   

    EchoClient(options).run()


if __name__ == '__main__':
    main()



