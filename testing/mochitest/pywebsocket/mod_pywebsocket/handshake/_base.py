





























"""Web Socket handshaking.

Note: request.connection.write/read are used in this module, even though
mod_python document says that they should be used only in connection handlers.
Unfortunately, we have no other options. For example, request.write/read are
not suitable because they don't allow direct raw bytes writing/reading.
"""


DEFAULT_WEB_SOCKET_PORT = 80
DEFAULT_WEB_SOCKET_SECURE_PORT = 443
WEB_SOCKET_SCHEME = 'ws'
WEB_SOCKET_SECURE_SCHEME = 'wss'


class HandshakeError(Exception):
    """Exception in Web Socket Handshake."""

    pass


def default_port(is_secure):
    if is_secure:
        return DEFAULT_WEB_SOCKET_SECURE_PORT
    else:
        return DEFAULT_WEB_SOCKET_PORT


def validate_protocol(protocol):
    """Validate WebSocket-Protocol string."""

    if not protocol:
        raise HandshakeError('Invalid WebSocket-Protocol: empty')
    for c in protocol:
        if not 0x20 <= ord(c) <= 0x7e:
            raise HandshakeError('Illegal character in protocol: %r' % c)


def parse_host_header(request):
    fields = request.headers_in['Host'].split(':', 1)
    if len(fields) == 1:
        return fields[0], default_port(request.is_https())
    try:
        return fields[0], int(fields[1])
    except ValueError, e:
        raise HandshakeError('Invalid port number format: %r' % e)


def build_location(request):
    """Build WebSocket location for request."""
    location_parts = []
    if request.is_https():
        location_parts.append(WEB_SOCKET_SECURE_SCHEME)
    else:
        location_parts.append(WEB_SOCKET_SCHEME)
    location_parts.append('://')
    host, port = parse_host_header(request)
    connection_port = request.connection.local_addr[1]
    if port != connection_port:
        raise HandshakeError('Header/connection port mismatch: %d/%d' %
                             (port, connection_port))
    location_parts.append(host)
    if (port != default_port(request.is_https())):
        location_parts.append(':')
        location_parts.append(str(port))
    location_parts.append(request.uri)
    return ''.join(location_parts)




