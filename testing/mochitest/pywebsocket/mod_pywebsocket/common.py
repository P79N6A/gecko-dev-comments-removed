






























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


VERSION_HYBI_LATEST = VERSION_HYBI10


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


DEFLATE_STREAM_EXTENSION = 'deflate-stream'
DEFLATE_FRAME_EXTENSION = 'deflate-frame'





STATUS_NORMAL = 1000
STATUS_GOING_AWAY = 1001
STATUS_PROTOCOL_ERROR = 1002
STATUS_UNSUPPORTED = 1003
STATUS_TOO_LARGE = 1004
STATUS_CODE_NOT_AVAILABLE = 1005
STATUS_ABNORMAL_CLOSE = 1006
STATUS_INVALID_UTF8 = 1007


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



