

from datetime import datetime, timedelta
import math
import sys

if sys.platform.startswith('win'):
    from terminal_win import Terminal
else:
    from terminal_unix import Terminal

class NullProgressBar(object):
    def update(self, current, data): pass
    def poke(self): pass
    def finish(self, complete=True): pass
    def beginline(self): pass
    def message(self, msg): sys.stdout.write(msg + '\n')
    @staticmethod
    def update_granularity(): return timedelta.max

class ProgressBar(object):
    def __init__(self, limit, fmt):
        assert self.conservative_isatty()

        self.prior = None
        self.atLineStart = True
        self.counters_fmt = fmt 
                                
        self.limit = limit 
        self.limit_digits = int(math.ceil(math.log10(self.limit))) 
        self.t0 = datetime.now() 

        
        self.counters_width = 1 
        for layout in self.counters_fmt:
            self.counters_width += self.limit_digits
            self.counters_width += 1 

        self.barlen = 64 - self.counters_width

    @staticmethod
    def update_granularity():
        return timedelta(seconds=0.1)

    def update(self, current, data):
        
        self.prior = (current, data)
        self.atLineStart = False

        
        sys.stdout.write('\r[')
        for layout in self.counters_fmt:
            Terminal.set_color(layout['color'])
            sys.stdout.write(('{:' + str(self.limit_digits) + 'd}').format(
                data[layout['value']]))
            Terminal.reset_color()
            if layout != self.counters_fmt[-1]:
                sys.stdout.write('|')
            else:
                sys.stdout.write('] ')

        
        pct = int(100.0 * current / self.limit)
        sys.stdout.write('{:3d}% '.format(pct))

        barlen = int(1.0 * self.barlen * current / self.limit) - 1
        bar = '=' * barlen + '>' + ' ' * (self.barlen - barlen - 1)
        sys.stdout.write(bar + '|')

        
        dt = datetime.now() - self.t0
        dt = dt.seconds + dt.microseconds * 1e-6
        sys.stdout.write('{:6.1f}s'.format(dt))
        Terminal.clear_right()

        
        sys.stdout.flush()

    def poke(self):
        if not self.prior:
            return
        self.update(*self.prior)

    def finish(self, complete=True):
        final_count = self.limit if complete else self.prior[0]
        self.update(final_count, self.prior[1])
        sys.stdout.write('\n')

    def beginline(self):
        if not self.atLineStart:
            sys.stdout.write('\n')
            self.atLineStart = True

    def message(self, msg):
        self.beginline()
        sys.stdout.write(msg)
        sys.stdout.write('\n')

    @staticmethod
    def conservative_isatty():
        """
        Prefer erring on the side of caution and not using terminal commands if
        the current output stream may be a file.  We explicitly check for the
        Android platform because terminal commands work poorly over ADB's
        redirection.
        """
        try:
            import android
            return False
        except ImportError:
            return sys.stdout.isatty()
        return False
