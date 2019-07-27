import datetime
import sys
import socket
from socket import timeout as SocketTimeout
import warnings
from .packages import six

try:  
    from http.client import HTTPConnection as _HTTPConnection, HTTPException
except ImportError:
    from httplib import HTTPConnection as _HTTPConnection, HTTPException


class DummyConnection(object):
    "Used to detect a failed ConnectionCls import."
    pass


try:  
    HTTPSConnection = DummyConnection
    import ssl
    BaseSSLError = ssl.SSLError
except (ImportError, AttributeError):  
    ssl = None

    class BaseSSLError(BaseException):
        pass


try:  
    
    ConnectionError = ConnectionError
except NameError:  
    class ConnectionError(Exception):
        pass


from .exceptions import (
    ConnectTimeoutError,
    SystemTimeWarning,
    SecurityWarning,
)
from .packages.ssl_match_hostname import match_hostname

from .util.ssl_ import (
    resolve_cert_reqs,
    resolve_ssl_version,
    ssl_wrap_socket,
    assert_fingerprint,
)


from .util import connection

port_by_scheme = {
    'http': 80,
    'https': 443,
}

RECENT_DATE = datetime.date(2014, 1, 1)


class HTTPConnection(_HTTPConnection, object):
    """
    Based on httplib.HTTPConnection but provides an extra constructor
    backwards-compatibility layer between older and newer Pythons.

    Additional keyword parameters are used to configure attributes of the connection.
    Accepted parameters include:

      - ``strict``: See the documentation on :class:`urllib3.connectionpool.HTTPConnectionPool`
      - ``source_address``: Set the source address for the current connection.

        .. note:: This is ignored for Python 2.6. It is only applied for 2.7 and 3.x

      - ``socket_options``: Set specific options on the underlying socket. If not specified, then
        defaults are loaded from ``HTTPConnection.default_socket_options`` which includes disabling
        Nagle's algorithm (sets TCP_NODELAY to 1) unless the connection is behind a proxy.

        For example, if you wish to enable TCP Keep Alive in addition to the defaults,
        you might pass::

            HTTPConnection.default_socket_options + [
                (socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1),
            ]

        Or you may want to disable the defaults by passing an empty list (e.g., ``[]``).
    """

    default_port = port_by_scheme['http']

    
    
    default_socket_options = [(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)]

    
    is_verified = False

    def __init__(self, *args, **kw):
        if six.PY3:  
            kw.pop('strict', None)

        
        self.source_address = kw.get('source_address')

        if sys.version_info < (2, 7):  
            
            
            
            
            kw.pop('source_address', None)

        
        
        self.socket_options = kw.pop('socket_options', self.default_socket_options)

        
        _HTTPConnection.__init__(self, *args, **kw)

    def _new_conn(self):
        """ Establish a socket connection and set nodelay settings on it.

        :return: New socket connection.
        """
        extra_kw = {}
        if self.source_address:
            extra_kw['source_address'] = self.source_address

        if self.socket_options:
            extra_kw['socket_options'] = self.socket_options

        try:
            conn = connection.create_connection(
                (self.host, self.port), self.timeout, **extra_kw)

        except SocketTimeout:
            raise ConnectTimeoutError(
                self, "Connection to %s timed out. (connect timeout=%s)" %
                (self.host, self.timeout))

        return conn

    def _prepare_conn(self, conn):
        self.sock = conn
        
        
        
        if getattr(self, '_tunnel_host', None):
            
            self._tunnel()
            
            self.auto_open = 0

    def connect(self):
        conn = self._new_conn()
        self._prepare_conn(conn)


class HTTPSConnection(HTTPConnection):
    default_port = port_by_scheme['https']

    def __init__(self, host, port=None, key_file=None, cert_file=None,
                 strict=None, timeout=socket._GLOBAL_DEFAULT_TIMEOUT, **kw):

        HTTPConnection.__init__(self, host, port, strict=strict,
                                timeout=timeout, **kw)

        self.key_file = key_file
        self.cert_file = cert_file

        
        
        self._protocol = 'https'

    def connect(self):
        conn = self._new_conn()
        self._prepare_conn(conn)
        self.sock = ssl.wrap_socket(conn, self.key_file, self.cert_file)


class VerifiedHTTPSConnection(HTTPSConnection):
    """
    Based on httplib.HTTPSConnection but wraps the socket with
    SSL certification.
    """
    cert_reqs = None
    ca_certs = None
    ssl_version = None
    assert_fingerprint = None

    def set_cert(self, key_file=None, cert_file=None,
                 cert_reqs=None, ca_certs=None,
                 assert_hostname=None, assert_fingerprint=None):

        self.key_file = key_file
        self.cert_file = cert_file
        self.cert_reqs = cert_reqs
        self.ca_certs = ca_certs
        self.assert_hostname = assert_hostname
        self.assert_fingerprint = assert_fingerprint

    def connect(self):
        
        conn = self._new_conn()

        resolved_cert_reqs = resolve_cert_reqs(self.cert_reqs)
        resolved_ssl_version = resolve_ssl_version(self.ssl_version)

        hostname = self.host
        if getattr(self, '_tunnel_host', None):
            
            

            self.sock = conn
            
            
            self._tunnel()
            
            self.auto_open = 0

            
            hostname = self._tunnel_host

        is_time_off = datetime.date.today() < RECENT_DATE
        if is_time_off:
            warnings.warn((
                'System time is way off (before {0}). This will probably '
                'lead to SSL verification errors').format(RECENT_DATE),
                SystemTimeWarning
            )

        
        
        self.sock = ssl_wrap_socket(conn, self.key_file, self.cert_file,
                                    cert_reqs=resolved_cert_reqs,
                                    ca_certs=self.ca_certs,
                                    server_hostname=hostname,
                                    ssl_version=resolved_ssl_version)

        if self.assert_fingerprint:
            assert_fingerprint(self.sock.getpeercert(binary_form=True),
                               self.assert_fingerprint)
        elif resolved_cert_reqs != ssl.CERT_NONE \
                and self.assert_hostname is not False:
            cert = self.sock.getpeercert()
            if not cert.get('subjectAltName', ()):
                warnings.warn((
                    'Certificate has no `subjectAltName`, falling back to check for a `commonName` for now. '
                    'This feature is being removed by major browsers and deprecated by RFC 2818. '
                    '(See https://github.com/shazow/urllib3/issues/497 for details.)'),
                    SecurityWarning
                )
            match_hostname(cert, self.assert_hostname or hostname)

        self.is_verified = (resolved_cert_reqs == ssl.CERT_REQUIRED
                            or self.assert_fingerprint is not None)


if ssl:
    
    UnverifiedHTTPSConnection = HTTPSConnection
    HTTPSConnection = VerifiedHTTPSConnection
