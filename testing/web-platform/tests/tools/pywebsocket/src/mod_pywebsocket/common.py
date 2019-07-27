





























"""This file must not depend on any module specific to the WebSocket protocol.
"""


from mod_pywebsocket import http_header_util



LOGLEVEL_FINE = 9


VERSION_HIXIE75 = -1
VERSION_HYBI00 = 0
VERSION_HYBI01 = 1
VERSION_HYBI02 = 2
VERSION_HYBI03 = 2
VERSION_HYBI04 = 4
VERSION_HYBI05 = 5
VERSION_HYBI06 = 6
VERSION_HYBI07 = 7
VERSION_HYBI08 = 8
VERSION_HYBI09 = 8
VERSION_HYBI10 = 8
VERSION_HYBI11 = 8
VERSION_HYBI12 = 8
VERSION_HYBI13 = 13
VERSION_HYBI14 = 13
VERSION_HYBI15 = 13
VERSION_HYBI16 = 13
VERSION_HYBI17 = 13


VERSION_HYBI_LATEST = VERSION_HYBI13


DEFAULT_WEB_SOCKET_PORT = 80
DEFAULT_WEB_SOCKET_SECURE_PORT = 443


WEB_SOCKET_SCHEME = 'ws'
WEB_SOCKET_SECURE_SCHEME = 'wss'


OPCODE_CONTINUATION = 0x0
OPCODE_TEXT = 0x1
OPCODE_BINARY = 0x2
OPCODE_CLOSE = 0x8
OPCODE_PING = 0x9
OPCODE_PONG = 0xa


WEBSOCKET_ACCEPT_UUID = '258EAFA5-E914-47DA-95CA-C5AB0DC85B11'


UPGRADE_HEADER = 'Upgrade'
WEBSOCKET_UPGRADE_TYPE = 'websocket'
WEBSOCKET_UPGRADE_TYPE_HIXIE75 = 'WebSocket'
CONNECTION_HEADER = 'Connection'
UPGRADE_CONNECTION_TYPE = 'Upgrade'
HOST_HEADER = 'Host'
ORIGIN_HEADER = 'Origin'
SEC_WEBSOCKET_ORIGIN_HEADER = 'Sec-WebSocket-Origin'
SEC_WEBSOCKET_KEY_HEADER = 'Sec-WebSocket-Key'
SEC_WEBSOCKET_ACCEPT_HEADER = 'Sec-WebSocket-Accept'
SEC_WEBSOCKET_VERSION_HEADER = 'Sec-WebSocket-Version'
SEC_WEBSOCKET_PROTOCOL_HEADER = 'Sec-WebSocket-Protocol'
SEC_WEBSOCKET_EXTENSIONS_HEADER = 'Sec-WebSocket-Extensions'
SEC_WEBSOCKET_DRAFT_HEADER = 'Sec-WebSocket-Draft'
SEC_WEBSOCKET_KEY1_HEADER = 'Sec-WebSocket-Key1'
SEC_WEBSOCKET_KEY2_HEADER = 'Sec-WebSocket-Key2'
SEC_WEBSOCKET_LOCATION_HEADER = 'Sec-WebSocket-Location'


DEFLATE_FRAME_EXTENSION = 'deflate-frame'
PERFRAME_COMPRESSION_EXTENSION = 'perframe-compress'
PERMESSAGE_COMPRESSION_EXTENSION = 'permessage-compress'
PERMESSAGE_DEFLATE_EXTENSION = 'permessage-deflate'
X_WEBKIT_DEFLATE_FRAME_EXTENSION = 'x-webkit-deflate-frame'
X_WEBKIT_PERMESSAGE_COMPRESSION_EXTENSION = 'x-webkit-permessage-compress'
MUX_EXTENSION = 'mux_DO_NOT_USE'










STATUS_NORMAL_CLOSURE = 1000
STATUS_GOING_AWAY = 1001
STATUS_PROTOCOL_ERROR = 1002
STATUS_UNSUPPORTED_DATA = 1003
STATUS_NO_STATUS_RECEIVED = 1005
STATUS_ABNORMAL_CLOSURE = 1006
STATUS_INVALID_FRAME_PAYLOAD_DATA = 1007
STATUS_POLICY_VIOLATION = 1008
STATUS_MESSAGE_TOO_BIG = 1009
STATUS_MANDATORY_EXTENSION = 1010
STATUS_INTERNAL_ENDPOINT_ERROR = 1011
STATUS_TLS_HANDSHAKE = 1015
STATUS_USER_REGISTERED_BASE = 3000
STATUS_USER_REGISTERED_MAX = 3999
STATUS_USER_PRIVATE_BASE = 4000
STATUS_USER_PRIVATE_MAX = 4999


STATUS_NORMAL = STATUS_NORMAL_CLOSURE
STATUS_UNSUPPORTED = STATUS_UNSUPPORTED_DATA
STATUS_CODE_NOT_AVAILABLE = STATUS_NO_STATUS_RECEIVED
STATUS_ABNORMAL_CLOSE = STATUS_ABNORMAL_CLOSURE
STATUS_INVALID_FRAME_PAYLOAD = STATUS_INVALID_FRAME_PAYLOAD_DATA
STATUS_MANDATORY_EXT = STATUS_MANDATORY_EXTENSION


HTTP_STATUS_BAD_REQUEST = 400
HTTP_STATUS_FORBIDDEN = 403
HTTP_STATUS_NOT_FOUND = 404


def is_control_opcode(opcode):
    return (opcode >> 3) == 1


class ExtensionParameter(object):
    """Holds information about an extension which is exchanged on extension
    negotiation in opening handshake.
    """

    def __init__(self, name):
        self._name = name
        
        
        
        
        self._parameters = []

    def name(self):
        return self._name

    def add_parameter(self, name, value):
        self._parameters.append((name, value))

    def get_parameters(self):
        return self._parameters

    def get_parameter_names(self):
        return [name for name, unused_value in self._parameters]

    def has_parameter(self, name):
        for param_name, param_value in self._parameters:
            if param_name == name:
                return True
        return False

    def get_parameter_value(self, name):
        for param_name, param_value in self._parameters:
            if param_name == name:
                return param_value


class ExtensionParsingException(Exception):
    def __init__(self, name):
        super(ExtensionParsingException, self).__init__(name)


def _parse_extension_param(state, definition, allow_quoted_string):
    param_name = http_header_util.consume_token(state)

    if param_name is None:
        raise ExtensionParsingException('No valid parameter name found')

    http_header_util.consume_lwses(state)

    if not http_header_util.consume_string(state, '='):
        definition.add_parameter(param_name, None)
        return

    http_header_util.consume_lwses(state)

    if allow_quoted_string:
        
        param_value = http_header_util.consume_token_or_quoted_string(state)
    else:
        param_value = http_header_util.consume_token(state)
    if param_value is None:
        raise ExtensionParsingException(
            'No valid parameter value found on the right-hand side of '
            'parameter %r' % param_name)

    definition.add_parameter(param_name, param_value)


def _parse_extension(state, allow_quoted_string):
    extension_token = http_header_util.consume_token(state)
    if extension_token is None:
        return None

    extension = ExtensionParameter(extension_token)

    while True:
        http_header_util.consume_lwses(state)

        if not http_header_util.consume_string(state, ';'):
            break

        http_header_util.consume_lwses(state)

        try:
            _parse_extension_param(state, extension, allow_quoted_string)
        except ExtensionParsingException, e:
            raise ExtensionParsingException(
                'Failed to parse parameter for %r (%r)' %
                (extension_token, e))

    return extension


def parse_extensions(data, allow_quoted_string=False):
    """Parses Sec-WebSocket-Extensions header value returns a list of
    ExtensionParameter objects.

    Leading LWSes must be trimmed.
    """

    state = http_header_util.ParsingState(data)

    extension_list = []
    while True:
        extension = _parse_extension(state, allow_quoted_string)
        if extension is not None:
            extension_list.append(extension)

        http_header_util.consume_lwses(state)

        if http_header_util.peek(state) is None:
            break

        if not http_header_util.consume_string(state, ','):
            raise ExtensionParsingException(
                'Failed to parse Sec-WebSocket-Extensions header: '
                'Expected a comma but found %r' %
                http_header_util.peek(state))

        http_header_util.consume_lwses(state)

    if len(extension_list) == 0:
        raise ExtensionParsingException(
            'No valid extension entry found')

    return extension_list


def format_extension(extension):
    """Formats an ExtensionParameter object."""

    formatted_params = [extension.name()]
    for param_name, param_value in extension.get_parameters():
        if param_value is None:
            formatted_params.append(param_name)
        else:
            quoted_value = http_header_util.quote_if_necessary(param_value)
            formatted_params.append('%s=%s' % (param_name, quoted_value))
    return '; '.join(formatted_params)


def format_extensions(extension_list):
    """Formats a list of ExtensionParameter objects."""

    formatted_extension_list = []
    for extension in extension_list:
        formatted_extension_list.append(format_extension(extension))
    return ', '.join(formatted_extension_list)



