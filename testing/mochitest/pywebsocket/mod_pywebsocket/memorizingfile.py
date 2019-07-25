































"""Memorizing file.

A memorizing file wraps a file and memorizes lines read by readline.
"""


import sys


class MemorizingFile(object):
    """MemorizingFile wraps a file and memorizes lines read by readline.

    Note that data read by other methods are not memorized. This behavior
    is good enough for memorizing lines SimpleHTTPServer reads before
    the control reaches WebSocketRequestHandler.
    """

    def __init__(self, file_, max_memorized_lines=sys.maxint):
        """Construct an instance.

        Args:
            file_: the file object to wrap.
            max_memorized_lines: the maximum number of lines to memorize.
                Only the first max_memorized_lines are memorized.
                Default: sys.maxint.
        """

        self._file = file_
        self._memorized_lines = []
        self._max_memorized_lines = max_memorized_lines
        self._buffered = False
        self._buffered_line = None

    def __getattribute__(self, name):
        if name in ('_file', '_memorized_lines', '_max_memorized_lines',
                    '_buffered', '_buffered_line', 'readline',
                    'get_memorized_lines'):
            return object.__getattribute__(self, name)
        return self._file.__getattribute__(name)

    def readline(self, size=-1):
        """Override file.readline and memorize the line read.

        Note that even if size is specified and smaller than actual size,
        the whole line will be read out from underlying file object by
        subsequent readline calls.
        """

        if self._buffered:
            line = self._buffered_line
            self._buffered = False
        else:
            line = self._file.readline()
            if line and len(self._memorized_lines) < self._max_memorized_lines:
                self._memorized_lines.append(line)
        if size >= 0 and size < len(line):
            self._buffered = True
            self._buffered_line = line[size:]
            return line[:size]
        return line

    def get_memorized_lines(self):
        """Get lines memorized so far."""
        return self._memorized_lines



