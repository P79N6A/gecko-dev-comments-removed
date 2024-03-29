
from mod_pywebsocket import msgutil

_GOODBYE_MESSAGE = u'Goodbye'

def web_socket_do_extra_handshake(request):
    
    
    if request.ws_requested_protocols:
        if "echo" in request.ws_requested_protocols:
            request.ws_protocol = "echo"


def web_socket_transfer_data(request):
    while True:
        line = request.ws_stream.receive_message()
        if line is None:
            return
        if isinstance(line, unicode):
            request.ws_stream.send_message(line, binary=False)
            if line == _GOODBYE_MESSAGE:
                return
        else:
            request.ws_stream.send_message(line, binary=True)

