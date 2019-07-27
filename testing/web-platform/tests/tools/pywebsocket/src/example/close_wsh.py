




























import struct

from mod_pywebsocket import common
from mod_pywebsocket import stream


def web_socket_do_extra_handshake(request):
    pass


def web_socket_transfer_data(request):
    while True:
        line = request.ws_stream.receive_message()
        if line is None:
            return
        code, reason = line.split(' ', 1)
        if code is None or reason is None:
            return
        request.ws_stream.close_connection(int(code), reason)
        
        
        
        
        
        
        


def web_socket_passive_closing_handshake(request):
    
    code, reason = request.ws_close_code, request.ws_close_reason

    
    if code == common.STATUS_NO_STATUS_RECEIVED:
        code = None
        reason = ''
    return code, reason



