
from mod_pywebsocket import msgutil

_GOODBYE_MESSAGE = u'Goodbye'

def web_socket_do_extra_handshake(request):
    
    

    pass  


def web_socket_transfer_data(request):
    while True:
        line = request.ws_stream.receive_message()
        if line is None:
            return
        if isinstance(line, unicode):
            if line == _GOODBYE_MESSAGE:
                return

