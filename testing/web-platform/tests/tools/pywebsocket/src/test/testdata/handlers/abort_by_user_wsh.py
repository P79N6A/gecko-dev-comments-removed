





























from mod_pywebsocket import handshake


def web_socket_do_extra_handshake(request):
    raise handshake.AbortedByUserException("abort for test")


def web_socket_transfer_data(request):
    raise handshake.AbortedByUserException("abort for test")



