





























def web_socket_do_extra_handshake(request):
    request.extra_headers.append(
        ('Strict-Transport-Security', 'max-age=86400'))


def web_socket_transfer_data(request):
    request.ws_stream.send_message('Hello', binary=False)



