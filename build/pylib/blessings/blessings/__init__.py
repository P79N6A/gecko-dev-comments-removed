from collections import defaultdict
import curses
from curses import tigetstr, tigetnum, setupterm, tparm
from fcntl import ioctl
try:
    from io import UnsupportedOperation as IOUnsupportedOperation
except ImportError:
    class IOUnsupportedOperation(Exception):
        """A dummy exception to take the place of Python 3's ``io.UnsupportedOperation`` in Python 2"""
        pass
import os
from os import isatty, environ
from platform import python_version_tuple
import struct
import sys
from termios import TIOCGWINSZ


if ('3', '0', '0') <= python_version_tuple() < ('3', '2', '2+'):  
    
    raise ImportError('Blessings needs Python 3.2.3 or greater for Python 3 '
                      'support due to http://bugs.python.org/issue10570.')


__all__ = ['Terminal']


class Terminal(object):
    """An abstraction around terminal capabilities

    Unlike curses, this doesn't require clearing the screen before doing
    anything, and it's friendlier to use. It keeps the endless calls to
    ``tigetstr()`` and ``tparm()`` out of your code, and it acts intelligently
    when somebody pipes your output to a non-terminal.

    Instance attributes:

      ``stream``
        The stream the terminal outputs to. It's convenient to pass the stream
        around with the terminal; it's almost always needed when the terminal
        is and saves sticking lots of extra args on client functions in
        practice.
      ``is_a_tty``
        Whether ``stream`` appears to be a terminal. You can examine this value
        to decide whether to draw progress bars or other frippery.

    """
    def __init__(self, kind=None, stream=None, force_styling=False):
        """Initialize the terminal.

        If ``stream`` is not a tty, I will default to returning an empty
        Unicode string for all capability values, so things like piping your
        output to a file won't strew escape sequences all over the place. The
        ``ls`` command sets a precedent for this: it defaults to columnar
        output when being sent to a tty and one-item-per-line when not.

        :arg kind: A terminal string as taken by ``setupterm()``. Defaults to
            the value of the ``TERM`` environment variable.
        :arg stream: A file-like object representing the terminal. Defaults to
            the original value of stdout, like ``curses.initscr()`` does.
        :arg force_styling: Whether to force the emission of capabilities, even
            if we don't seem to be in a terminal. This comes in handy if users
            are trying to pipe your output through something like ``less -r``,
            which supports terminal codes just fine but doesn't appear itself
            to be a terminal. Just expose a command-line option, and set
            ``force_styling`` based on it. Terminal initialization sequences
            will be sent to ``stream`` if it has a file descriptor and to
            ``sys.__stdout__`` otherwise. (``setupterm()`` demands to send them
            somewhere, and stdout is probably where the output is ultimately
            headed. If not, stderr is probably bound to the same terminal.)

        """
        if stream is None:
            stream = sys.__stdout__
        try:
            stream_descriptor = (stream.fileno() if hasattr(stream, 'fileno')
                                                 and callable(stream.fileno)
                                 else None)
        except IOUnsupportedOperation:
            stream_descriptor = None

        self.is_a_tty = stream_descriptor is not None and isatty(stream_descriptor)
        self._does_styling = self.is_a_tty or force_styling

        
        
        
        self._init_descriptor = (sys.__stdout__.fileno()
                                 if stream_descriptor is None
                                 else stream_descriptor)
        if self._does_styling:
            
            
            
            
            
            setupterm(kind or environ.get('TERM', 'unknown'),
                      self._init_descriptor)

        self.stream = stream

    
    
    _sugar = dict(
        
        
        
        save='sc',
        restore='rc',

        clear_eol='el',
        clear_bol='el1',
        clear_eos='ed',
        position='cup',  
        move='cup',
        move_x='hpa',
        move_y='vpa',

        reset_colors='op',  

        normal='sgr0',
        reverse='rev',
        
        
        
        
        italic='sitm',
        no_italic='ritm',
        shadow='sshm',
        no_shadow='rshm',
        standout='smso',
        no_standout='rmso',
        subscript='ssubm',
        no_subscript='rsubm',
        superscript='ssupm',
        no_superscript='rsupm',
        underline='smul',
        no_underline='rmul')

    def __getattr__(self, attr):
        """Return parametrized terminal capabilities, like bold.

        For example, you can say ``term.bold`` to get the string that turns on
        bold formatting and ``term.normal`` to get the string that turns it off
        again. Or you can take a shortcut: ``term.bold('hi')`` bolds its
        argument and sets everything to normal afterward. You can even combine
        things: ``term.bold_underline_red_on_bright_green('yowzers!')``.

        For a parametrized capability like ``cup``, pass the parameters too:
        ``some_term.cup(line, column)``.

        ``man terminfo`` for a complete list of capabilities.

        Return values are always Unicode.

        """
        resolution = self._resolve_formatter(attr) if self._does_styling else NullCallableString()
        setattr(self, attr, resolution)  
        return resolution

    @property
    def height(self):
        """The height of the terminal in characters

        If no stream or a stream not representing a terminal was passed in at
        construction, return the dimension of the controlling terminal so
        piping to things that eventually display on the terminal (like ``less
        -R``) work. If a stream representing a terminal was passed in, return
        the dimensions of that terminal. If there somehow is no controlling
        terminal, return ``None``. (Thus, you should check that ``is_a_tty`` is
        true before doing any math on the result.)

        """
        return self._height_and_width()[0]

    @property
    def width(self):
        """The width of the terminal in characters

        See ``height()`` for some corner cases.

        """
        return self._height_and_width()[1]

    def _height_and_width(self):
        """Return a tuple of (terminal height, terminal width)."""
        
        
        for descriptor in self._init_descriptor, sys.__stdout__:
            try:
                return struct.unpack('hhhh', ioctl(descriptor, TIOCGWINSZ, '\000' * 8))[0:2]
            except IOError:
                pass
        return None, None  

    def location(self, x=None, y=None):
        """Return a context manager for temporarily moving the cursor.

        Move the cursor to a certain position on entry, let you print stuff
        there, then return the cursor to its original position::

            term = Terminal()
            with term.location(2, 5):
                print 'Hello, world!'
                for x in xrange(10):
                    print 'I can do it %i times!' % x

        Specify ``x`` to move to a certain column, ``y`` to move to a certain
        row, or both.

        """
        return Location(self, x, y)

    @property
    def color(self):
        """Return a capability that sets the foreground color.

        The capability is unparametrized until called and passed a number
        (0-15), at which point it returns another string which represents a
        specific color change. This second string can further be called to
        color a piece of text and set everything back to normal afterward.

        :arg num: The number, 0-15, of the color

        """
        return ParametrizingString(self._foreground_color, self.normal)

    @property
    def on_color(self):
        """Return a capability that sets the background color.

        See ``color()``.

        """
        return ParametrizingString(self._background_color, self.normal)

    @property
    def number_of_colors(self):
        """Return the number of colors the terminal supports.

        Common values are 0, 8, 16, 88, and 256.

        Though the underlying capability returns -1 when there is no color
        support, we return 0. This lets you test more Pythonically::

            if term.number_of_colors:
                ...

        We also return 0 if the terminal won't tell us how many colors it
        supports, which I think is rare.

        """
        
        
        
        
        colors = tigetnum('colors')  
        
        return colors if colors >= 0 else 0

    def _resolve_formatter(self, attr):
        """Resolve a sugary or plain capability name, color, or compound formatting function name into a callable capability."""
        if attr in COLORS:
            return self._resolve_color(attr)
        elif attr in COMPOUNDABLES:
            
            return self._formatting_string(self._resolve_capability(attr))
        else:
            formatters = split_into_formatters(attr)
            if all(f in COMPOUNDABLES for f in formatters):
                
                
                
                return self._formatting_string(
                    u''.join(self._resolve_formatter(s) for s in formatters))
            else:
                return ParametrizingString(self._resolve_capability(attr))

    def _resolve_capability(self, atom):
        """Return a terminal code for a capname or a sugary name, or an empty Unicode.

        The return value is always Unicode, because otherwise it is clumsy
        (especially in Python 3) to concatenate with real (Unicode) strings.

        """
        code = tigetstr(self._sugar.get(atom, atom))
        if code:
            
            
            
            return code.decode('utf-8')
        return u''

    def _resolve_color(self, color):
        """Resolve a color like red or on_bright_green into a callable capability."""
        
        
        
        
        color_cap = (self._background_color if 'on_' in color else
                     self._foreground_color)
        
        
        offset = 8 if 'bright_' in color else 0
        base_color = color.rsplit('_', 1)[-1]
        return self._formatting_string(
            color_cap(getattr(curses, 'COLOR_' + base_color.upper()) + offset))

    @property
    def _foreground_color(self):
        return self.setaf or self.setf

    @property
    def _background_color(self):
        return self.setab or self.setb

    def _formatting_string(self, formatting):
        """Return a new ``FormattingString`` which implicitly receives my notion of "normal"."""
        return FormattingString(formatting, self.normal)


def derivative_colors(colors):
    """Return the names of valid color variants, given the base colors."""
    return set([('on_' + c) for c in colors] +
               [('bright_' + c) for c in colors] +
               [('on_bright_' + c) for c in colors])


COLORS = set(['black', 'red', 'green', 'yellow', 'blue', 'magenta', 'cyan', 'white'])
COLORS.update(derivative_colors(COLORS))
COMPOUNDABLES = (COLORS |
                 set(['bold', 'underline', 'reverse', 'blink', 'dim', 'italic',
                      'shadow', 'standout', 'subscript', 'superscript']))


class ParametrizingString(unicode):
    """A Unicode string which can be called to parametrize it as a terminal capability"""
    def __new__(cls, formatting, normal=None):
        """Instantiate.

        :arg normal: If non-None, indicates that, once parametrized, this can
            be used as a ``FormattingString``. The value is used as the
            "normal" capability.

        """
        new = unicode.__new__(cls, formatting)
        new._normal = normal
        return new

    def __call__(self, *args):
        try:
            
            
            
            parametrized = tparm(self.encode('utf-8'), *args).decode('utf-8')
            return (parametrized if self._normal is None else
                    FormattingString(parametrized, self._normal))
        except curses.error:
            
            
            
            
            return u''
        except TypeError:
            
            
            if len(args) == 1 and isinstance(args[0], basestring):
                raise TypeError(
                    'A native or nonexistent capability template received '
                    '%r when it was expecting ints. You probably misspelled a '
                    'formatting call like bright_red_on_white(...).' % args)
            else:
                
                
                raise


class FormattingString(unicode):
    """A Unicode string which can be called upon a piece of text to wrap it in formatting"""
    def __new__(cls, formatting, normal):
        new = unicode.__new__(cls, formatting)
        new._normal = normal
        return new

    def __call__(self, text):
        """Return a new string that is ``text`` formatted with my contents.

        At the beginning of the string, I prepend the formatting that is my
        contents. At the end, I append the "normal" sequence to set everything
        back to defaults. The return value is always a Unicode.

        """
        return self + text + self._normal


class NullCallableString(unicode):
    """A dummy class to stand in for ``FormattingString`` and ``ParametrizingString``

    A callable bytestring that returns an empty Unicode when called with an int
    and the arg otherwise. We use this when there is no tty and so all
    capabilities are blank.

    """
    def __new__(cls):
        new = unicode.__new__(cls, u'')
        return new

    def __call__(self, arg):
        if isinstance(arg, int):
            return u''
        return arg  


def split_into_formatters(compound):
    """Split a possibly compound format string into segments.

    >>> split_into_formatters('bold_underline_bright_blue_on_red')
    ['bold', 'underline', 'bright_blue', 'on_red']

    """
    merged_segs = []
    
    mergeable_prefixes = ['on', 'bright', 'on_bright']
    for s in compound.split('_'):
        if merged_segs and merged_segs[-1] in mergeable_prefixes:
            merged_segs[-1] += '_' + s
        else:
            merged_segs.append(s)
    return merged_segs


class Location(object):
    """Context manager for temporarily moving the cursor"""
    def __init__(self, term, x=None, y=None):
        self.x, self.y = x, y
        self.term = term

    def __enter__(self):
        """Save position and move to the requested column, row, or both."""
        self.term.stream.write(self.term.save)  
        if self.x and self.y:
            self.term.stream.write(self.term.move(self.y, self.x))
        elif self.x:
            self.term.stream.write(self.term.move_x(self.x))
        elif self.y:
            self.term.stream.write(self.term.move_y(self.y))

    def __exit__(self, type, value, tb):
        """Restore original cursor position."""
        self.term.stream.write(self.term.restore)
