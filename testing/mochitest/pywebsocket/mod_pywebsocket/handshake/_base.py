





























"""Common functions and exceptions used by WebSocket opening handshake
processors.
"""


from mod_pywebsocket import common


class HandshakeError(Exception):
    """This exception will be raised when an error occurred while processing
    WebSocket initial handshake.
    """
    pass


def get_default_port(is_secure):
    if is_secure:
        return common.DEFAULT_WEB_SOCKET_SECURE_PORT
    else:
        return common.DEFAULT_WEB_SOCKET_PORT



def validate_subprotocol(subprotocol):
    """Validate a value in subprotocol fields such as WebSocket-Protocol,
    Sec-WebSocket-Protocol.

    See
    - HyBi 06: Section 5.2.2.
    - HyBi 00: Section 4.1. Opening handshake
    - Hixie 75: Section 4.1. Handshake
    """

    if not subprotocol:
        raise HandshakeError('Invalid subprotocol name: empty')
    for c in subprotocol:
        if not 0x20 <= ord(c) <= 0x7e:
            raise HandshakeError(
                'Illegal character in subprotocol name: %r' % c)


def parse_host_header(request):
    fields = request.headers_in['Host'].split(':', 1)
    if len(fields) == 1:
        return fields[0], get_default_port(request.is_https())
    try:
        return fields[0], int(fields[1])
    except ValueError, e:
        raise HandshakeError('Invalid port number format: %r' % e)


def build_location(request):
    """Build WebSocket location for request."""
    location_parts = []
    if request.is_https():
        location_parts.append(common.WEB_SOCKET_SECURE_SCHEME)
    else:
        location_parts.append(common.WEB_SOCKET_SCHEME)
    location_parts.append('://')
    host, port = parse_host_header(request)
    connection_port = request.connection.local_addr[1]
    if port != connection_port:
        raise HandshakeError('Header/connection port mismatch: %d/%d' %
                             (port, connection_port))
    location_parts.append(host)
    if (port != get_default_port(request.is_https())):
        location_parts.append(':')
        location_parts.append(str(port))
    location_parts.append(request.uri)
    return ''.join(location_parts)


def get_mandatory_header(request, key, expected_value=None):
    value = request.headers_in.get(key)
    if value is None:
        raise HandshakeError('Header %s is not defined' % key)
    if expected_value is not None and expected_value != value:
        raise HandshakeError(
            'Illegal value for header %s: %s (expected: %s)' %
            (key, value, expected_value))
    return value


def check_header_lines(request, mandatory_headers):
    
    
    if request.method != 'GET':
        raise HandshakeError('Method is not GET')
    
    
    
    for key, expected_value in mandatory_headers:
        get_mandatory_header(request, key, expected_value)



