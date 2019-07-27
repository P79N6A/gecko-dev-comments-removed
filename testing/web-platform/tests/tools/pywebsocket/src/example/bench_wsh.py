





























"""A simple load tester for WebSocket clients.

A client program sends a message formatted as "<time> <count> <message>" to
this handler. This handler starts sending total <count> WebSocket messages
containing <message> every <time> seconds. <time> can be a floating point
value. <count> must be an integer value.
"""


import time


def web_socket_do_extra_handshake(request):
    pass  


def web_socket_transfer_data(request):
    line = request.ws_stream.receive_message()
    parts = line.split(' ')
    if len(parts) != 3:
        raise ValueError('Bad parameter format')
    wait = float(parts[0])
    count = int(parts[1])
    message = parts[2]
    for i in xrange(count):
        request.ws_stream.send_message(message)
        time.sleep(wait)



