





























_GOODBYE_MESSAGE = u'Goodbye'


def web_socket_do_extra_handshake(request):
    """Received Sec-WebSocket-Extensions header value is parsed into
    request.ws_requested_extensions. pywebsocket creates extension
    processors using it before do_extra_handshake call and never looks at it
    after the call.

    To reject requested extensions, clear the processor list.
    """

    request.ws_extension_processors = []


def web_socket_transfer_data(request):
    """Echo. Same as echo_wsh.py."""

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



