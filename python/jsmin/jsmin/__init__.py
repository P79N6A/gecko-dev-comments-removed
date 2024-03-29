

























import sys
is_3 = sys.version_info >= (3, 0)
if is_3:
    import io
else:
    import StringIO
    try:
        import cStringIO
    except ImportError:
        cStringIO = None


__all__ = ['jsmin', 'JavascriptMinify']
__version__ = '2.0.11'


def jsmin(js, **kwargs):
    """
    returns a minified version of the javascript string
    """
    if not is_3:        
        if cStringIO and not isinstance(js, unicode):
            
            
            klass = cStringIO.StringIO
        else:
            klass = StringIO.StringIO
    else:
        klass = io.StringIO
    ins = klass(js)
    outs = klass()
    JavascriptMinify(ins, outs, **kwargs).minify()
    return outs.getvalue()


class JavascriptMinify(object):
    """
    Minify an input stream of javascript, writing
    to an output stream
    """

    def __init__(self, instream=None, outstream=None, quote_chars="'\""):
        self.ins = instream
        self.outs = outstream
        self.quote_chars = quote_chars

    def minify(self, instream=None, outstream=None):
        if instream and outstream:
            self.ins, self.outs = instream, outstream
        
        self.is_return = False
        self.return_buf = ''
        
        def write(char):
            
            
            if char in 'return':
                self.return_buf += char
                self.is_return = self.return_buf == 'return'
            self.outs.write(char)
            if self.is_return:
                self.return_buf = ''

        read = self.ins.read

        space_strings = "abcdefghijklmnopqrstuvwxyz"\
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_$\\"
        starters, enders = '{[(+-', '}])+-' + self.quote_chars
        newlinestart_strings = starters + space_strings
        newlineend_strings = enders + space_strings
        do_newline = False
        do_space = False
        escape_slash_count = 0
        doing_single_comment = False
        previous_before_comment = ''
        doing_multi_comment = False
        in_re = False
        in_quote = ''
        quote_buf = []
        
        previous = read(1)
        if previous == '\\':
            escape_slash_count += 1
        next1 = read(1)
        if previous == '/':
            if next1 == '/':
                doing_single_comment = True
            elif next1 == '*':
                doing_multi_comment = True
                previous = next1
                next1 = read(1)
            else:
                in_re = True  
                write(previous)
        elif not previous:
            return
        elif previous >= '!':
            if previous in self.quote_chars:
                in_quote = previous
            write(previous)
            previous_non_space = previous
        else:
            previous_non_space = ' '
        if not next1:
            return

        while 1:
            next2 = read(1)
            if not next2:
                last = next1.strip()
                if not (doing_single_comment or doing_multi_comment)\
                    and last not in ('', '/'):
                    if in_quote:
                        write(''.join(quote_buf))
                    write(last)
                break
            if doing_multi_comment:
                if next1 == '*' and next2 == '/':
                    doing_multi_comment = False
                    if previous_before_comment and previous_before_comment in space_strings:
                        do_space = True
                    next2 = read(1)
            elif doing_single_comment:
                if next1 in '\r\n':
                    doing_single_comment = False
                    while next2 in '\r\n':
                        next2 = read(1)
                        if not next2:
                            break
                    if previous_before_comment in ')}]':
                        do_newline = True
                    elif previous_before_comment in space_strings:
                        write('\n')
            elif in_quote:
                quote_buf.append(next1)

                if next1 == in_quote:
                    numslashes = 0
                    for c in reversed(quote_buf[:-1]):
                        if c != '\\':
                            break
                        else:
                            numslashes += 1
                    if numslashes % 2 == 0:
                        in_quote = ''
                        write(''.join(quote_buf))
            elif next1 in '\r\n':
                if previous_non_space in newlineend_strings \
                    or previous_non_space > '~':
                    while 1:
                        if next2 < '!':
                            next2 = read(1)
                            if not next2:
                                break
                        else:
                            if next2 in newlinestart_strings \
                                or next2 > '~' or next2 == '/':
                                do_newline = True
                            break
            elif next1 < '!' and not in_re:
                if (previous_non_space in space_strings \
                    or previous_non_space > '~') \
                    and (next2 in space_strings or next2 > '~'):
                    do_space = True
                elif previous_non_space in '-+' and next2 == previous_non_space:
                    
                    do_space = True
                elif self.is_return and next2 == '/':
                    
                    write(' ')
            elif next1 == '/':
                if do_space:
                    write(' ')
                if in_re:
                    if previous != '\\' or (not escape_slash_count % 2) or next2 in 'gimy':
                        in_re = False
                    write('/')
                elif next2 == '/':                    
                    doing_single_comment = True
                    previous_before_comment = previous_non_space
                elif next2 == '*':
                    doing_multi_comment = True
                    previous_before_comment = previous_non_space
                    previous = next1
                    next1 = next2
                    next2 = read(1)
                else:
                    in_re = previous_non_space in '(,=:[?!&|;' or self.is_return  
                    write('/')
            else:
                if do_space:
                    do_space = False
                    write(' ')
                if do_newline:
                    write('\n')
                    do_newline = False

                write(next1)
                if not in_re and next1 in self.quote_chars:
                    in_quote = next1
                    quote_buf = []

            previous = next1
            next1 = next2

            if previous >= '!':
                previous_non_space = previous

            if previous == '\\':
                escape_slash_count += 1
            else:
                escape_slash_count = 0
