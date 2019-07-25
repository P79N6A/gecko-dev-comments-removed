





























from mod_pywebsocket import common
from mod_pywebsocket import util


_available_processors = {}


class ExtensionProcessorInterface(object):
    def get_extension_response(self):
        return None

    def setup_stream_options(self, stream_options):
        pass


class DeflateStreamExtensionProcessor(ExtensionProcessorInterface):
    """WebSocket DEFLATE stream extension processor."""

    def __init__(self, request):
        self._logger = util.get_class_logger(self)

        self._request = request

    def get_extension_response(self):
        if len(self._request.get_parameter_names()) != 0:
            return None

        self._logger.debug(
            'Enable %s extension', common.DEFLATE_STREAM_EXTENSION)

        return common.ExtensionParameter(common.DEFLATE_STREAM_EXTENSION)

    def setup_stream_options(self, stream_options):
        stream_options.deflate_stream = True


_available_processors[common.DEFLATE_STREAM_EXTENSION] = (
    DeflateStreamExtensionProcessor)


class DeflateFrameExtensionProcessor(ExtensionProcessorInterface):
    """WebSocket Per-frame DEFLATE extension processor."""

    _WINDOW_BITS_PARAM = 'window_bits'
    _NO_CONTEXT_TAKEOVER_PARAM = 'no_context_takeover'

    def __init__(self, request):
        self._logger = util.get_class_logger(self)

        self._request = request

        self._response_window_bits = None
        self._response_no_context_takeover = False

    def get_extension_response(self):
        

        window_bits = self._request.get_parameter_value(
            self._WINDOW_BITS_PARAM)
        no_context_takeover = self._request.has_parameter(
            self._NO_CONTEXT_TAKEOVER_PARAM)
        if (no_context_takeover and
            self._request.get_parameter_value(
                self._NO_CONTEXT_TAKEOVER_PARAM) is not None):
            return None

        if window_bits is not None:
            try:
                window_bits = int(window_bits)
            except ValueError, e:
                return None
            if window_bits < 8 or window_bits > 15:
                return None

        self._deflater = util._RFC1979Deflater(
            window_bits, no_context_takeover)

        self._inflater = util._RFC1979Inflater()

        self._compress_outgoing = True

        response = common.ExtensionParameter(common.DEFLATE_FRAME_EXTENSION)

        if self._response_window_bits is not None:
            response.add_parameter(
                self._WINDOW_BITS_PARAM, str(self._response_window_bits))
        if self._response_no_context_takeover:
            response.add_parameter(
                self._NO_CONTEXT_TAKEOVER_PARAM, None)

        self._logger.debug(
            'Enable %s extension ('
            'request: window_bits=%s; no_context_takeover=%r, '
            'response: window_wbits=%s; no_context_takeover=%r)' %
            (common.DEFLATE_STREAM_EXTENSION,
             window_bits,
             no_context_takeover,
             self._response_window_bits,
             self._response_no_context_takeover))

        return response

    def setup_stream_options(self, stream_options):
        class _OutgoingFilter(object):
            def __init__(self, parent):
                self._parent = parent

            def filter(self, frame):
                self._parent._outgoing_filter(frame)

        class _IncomingFilter(object):
            def __init__(self, parent):
                self._parent = parent

            def filter(self, frame):
                self._parent._incoming_filter(frame)

        stream_options.outgoing_frame_filters.append(
            _OutgoingFilter(self))
        stream_options.incoming_frame_filters.insert(
            0, _IncomingFilter(self))

    def set_response_window_bits(self, value):
        self._response_window_bits = value

    def set_response_no_context_takeover(self, value):
        self._response_no_context_takeover = value

    def enable_outgoing_compression(self):
        self._compress_outgoing = True

    def disable_outgoing_compression(self):
        self._compress_outgoing = False

    def _outgoing_filter(self, frame):
        """Transform outgoing frames. This method is called only by
        an _OutgoingFilter instance.
        """

        if (not self._compress_outgoing or
            common.is_control_opcode(frame.opcode)):
            return

        frame.payload = self._deflater.filter(frame.payload)
        frame.rsv1 = 1

    def _incoming_filter(self, frame):
        """Transform incoming frames. This method is called only by
        an _IncomingFilter instance.
        """

        if frame.rsv1 != 1 or common.is_control_opcode(frame.opcode):
            return

        frame.payload = self._inflater.filter(frame.payload)
        frame.rsv1 = 0


_available_processors[common.DEFLATE_FRAME_EXTENSION] = (
    DeflateFrameExtensionProcessor)


def get_extension_processor(extension_request):
    global _available_processors
    processor_class = _available_processors.get(extension_request.name())
    if processor_class is None:
        return None
    return processor_class(extension_request)



