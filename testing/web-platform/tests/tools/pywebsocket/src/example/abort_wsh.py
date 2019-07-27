





























from mod_pywebsocket import handshake


def web_socket_do_extra_handshake(request):
    pass


def web_socket_transfer_data(request):
    raise handshake.AbortedByUserException(
        "Aborted in web_socket_transfer_data")



