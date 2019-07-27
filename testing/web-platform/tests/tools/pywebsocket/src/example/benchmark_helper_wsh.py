





























"""Handler for benchmark.html."""


def web_socket_do_extra_handshake(request):
    
    request.ws_extension_processors = []


def web_socket_transfer_data(request):
    data = ''

    while True:
        line = request.ws_stream.receive_message()
        if line is None:
            return

        if isinstance(line, unicode):
            
            size = int(line)
            if len(data) < size:
                data = 'a' * size
            request.ws_stream.send_message(data, binary=True)
        else:
            
            if line != 'a' * len(line):
                return
            request.ws_stream.send_message(str(len(line)));



