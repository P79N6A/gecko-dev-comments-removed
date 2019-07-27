





























"""Handler for benchmark.html."""


def web_socket_do_extra_handshake(request):
    
    request.ws_extension_processors = []


def web_socket_transfer_data(request):
    data = ''

    while True:
        command = request.ws_stream.receive_message()
        if command is None:
            return

        if not isinstance(command, unicode):
            raise ValueError('Invalid command data:' + command)
        commands = command.split(' ')
        if len(commands) == 0:
            raise ValueError('Invalid command data: ' + command)

        if commands[0] == 'receive':
            if len(commands) != 2:
                raise ValueError(
                        'Illegal number of arguments for send command' +
                        command)
            size = int(commands[1])

            
            if len(data) != size:
                data = 'a' * size
            request.ws_stream.send_message(data, binary=True)
        elif commands[0] == 'send':
            if len(commands) != 2:
                raise ValueError(
                        'Illegal number of arguments for receive command' +
                        command)
            verify_data = commands[1] == '1'

            data = request.ws_stream.receive_message()
            if data is None:
                raise ValueError('Payload not received')
            size = len(data)

            if verify_data:
                if data != 'a' * size:
                    raise ValueError('Payload verification failed')

            request.ws_stream.send_message(str(size))
        else:
            raise ValueError('Invalid command: ' + commands[0])



