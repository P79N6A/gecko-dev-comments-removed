





























"""Common functions and exceptions used by WebSocket opening handshake
processors.
"""


from mod_pywebsocket import common
from mod_pywebsocket import http_header_util


class AbortedByUserException(Exception):
    """Exception for aborting a connection intentionally.

    If this exception is raised in do_extra_handshake handler, the connection
    will be abandoned. No other WebSocket or HTTP(S) handler will be invoked.

    If this exception is raised in transfer_data_handler, the connection will
    be closed without closing handshake. No other WebSocket or HTTP(S) handler
    will be invoked.
    """

    pass


class HandshakeException(Exception):
    """This exception will be raised when an error occurred while processing
    WebSocket initial handshake.
    """

    def __init__(self, name, status=None):
        super(HandshakeException, self).__init__(name)
        self.status = status


class VersionException(Exception):
    """This exception will be raised when a version of client request does not
    match with version the server supports.
    """

    def __init__(self, name, supported_versions=''):
        """Construct an instance.

        Args:
            supported_version: a str object to show supported hybi versions.
                               (e.g. '8, 13')
        """
        super(VersionException, self).__init__(name)
        self.supported_versions = supported_versions


def get_default_port(is_secure):
    if is_secure:
        return common.DEFAULT_WEB_SOCKET_SECURE_PORT
    else:
        return common.DEFAULT_WEB_SOCKET_PORT


def validate_subprotocol(subprotocol):
    """Validate a value in the Sec-WebSocket-Protocol field.

    See the Section 4.1., 4.2.2., and 4.3. of RFC 6455.
    """

    if not subprotocol:
        raise HandshakeException('Invalid subprotocol name: empty')

    
    state = http_header_util.ParsingState(subprotocol)
    token = http_header_util.consume_token(state)
    rest = http_header_util.peek(state)
    
    
    
    if rest is not None:
        raise HandshakeException('Invalid non-token string in subprotocol '
                                 'name: %r' % rest)


def parse_host_header(request):
    fields = request.headers_in[common.HOST_HEADER].split(':', 1)
    if len(fields) == 1:
        return fields[0], get_default_port(request.is_https())
    try:
        return fields[0], int(fields[1])
    except ValueError, e:
        raise HandshakeException('Invalid port number format: %r' % e)


def format_header(name, value):
    return '%s: %s\r\n' % (name, value)


def get_mandatory_header(request, key):
    value = request.headers_in.get(key)
    if value is None:
        raise HandshakeException('Header %s is not defined' % key)
    return value


def validate_mandatory_header(request, key, expected_value, fail_status=None):
    value = get_mandatory_header(request, key)

    if value.lower() != expected_value.lower():
        raise HandshakeException(
            'Expected %r for header %s but found %r (case-insensitive)' %
            (expected_value, key, value), status=fail_status)


def check_request_line(request):
    
    
    if request.method != 'GET':
        raise HandshakeException('Method is not GET: %r' % request.method)

    if request.protocol != 'HTTP/1.1':
        raise HandshakeException('Version is not HTTP/1.1: %r' %
                                 request.protocol)


def parse_token_list(data):
    """Parses a header value which follows 1#token and returns parsed elements
    as a list of strings.

    Leading LWSes must be trimmed.
    """

    state = http_header_util.ParsingState(data)

    token_list = []

    while True:
        token = http_header_util.consume_token(state)
        if token is not None:
            token_list.append(token)

        http_header_util.consume_lwses(state)

        if http_header_util.peek(state) is None:
            break

        if not http_header_util.consume_string(state, ','):
            raise HandshakeException(
                'Expected a comma but found %r' % http_header_util.peek(state))

        http_header_util.consume_lwses(state)

    if len(token_list) == 0:
        raise HandshakeException('No valid token found')

    return token_list



