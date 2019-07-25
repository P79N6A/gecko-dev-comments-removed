
















"""Tokenize C++ source code."""

__author__ = 'nnorwitz@google.com (Neal Norwitz)'


try:
    
    import builtins
except ImportError:
    
    import __builtin__ as builtins


import sys

from cpp import utils


if not hasattr(builtins, 'set'):
    
    from sets import Set as set



_letters = 'abcdefghijklmnopqrstuvwxyz'
VALID_IDENTIFIER_CHARS = set(_letters + _letters.upper() + '_0123456789$')
HEX_DIGITS = set('0123456789abcdefABCDEF')
INT_OR_FLOAT_DIGITS = set('01234567890eE-+')



_STR_PREFIXES = set(('R', 'u8', 'u8R', 'u', 'uR', 'U', 'UR', 'L', 'LR'))



UNKNOWN = 'UNKNOWN'
SYNTAX = 'SYNTAX'
CONSTANT = 'CONSTANT'
NAME = 'NAME'
PREPROCESSOR = 'PREPROCESSOR'



WHENCE_STREAM, WHENCE_QUEUE = range(2)


class Token(object):
    """Data container to represent a C++ token.

    Tokens can be identifiers, syntax char(s), constants, or
    pre-processor directives.

    start contains the index of the first char of the token in the source
    end contains the index of the last char of the token in the source
    """

    def __init__(self, token_type, name, start, end):
        self.token_type = token_type
        self.name = name
        self.start = start
        self.end = end
        self.whence = WHENCE_STREAM

    def __str__(self):
        if not utils.DEBUG:
            return 'Token(%r)' % self.name
        return 'Token(%r, %s, %s)' % (self.name, self.start, self.end)

    __repr__ = __str__


def _GetString(source, start, i):
    i = source.find('"', i+1)
    while source[i-1] == '\\':
        
        backslash_count = 1
        j = i - 2
        while source[j] == '\\':
            backslash_count += 1
            j -= 1
        
        if (backslash_count % 2) == 0:
            break
        i = source.find('"', i+1)
    return i + 1


def _GetChar(source, start, i):
    
    i = source.find("'", i+1)
    while source[i-1] == '\\':
        
        if (i - 2) > start and source[i-2] == '\\':
            break
        i = source.find("'", i+1)
    
    if i < 0:
        i = start
    return i + 1


def GetTokens(source):
    """Returns a sequence of Tokens.

    Args:
      source: string of C++ source code.

    Yields:
      Token that represents the next token in the source.
    """
    
    valid_identifier_chars = VALID_IDENTIFIER_CHARS
    hex_digits = HEX_DIGITS
    int_or_float_digits = INT_OR_FLOAT_DIGITS
    int_or_float_digits2 = int_or_float_digits | set('.')

    
    ignore_errors = False
    count_ifs = 0

    i = 0
    end = len(source)
    while i < end:
        
        while i < end and source[i].isspace():
            i += 1
        if i >= end:
            return

        token_type = UNKNOWN
        start = i
        c = source[i]
        if c.isalpha() or c == '_':              
            token_type = NAME
            while source[i] in valid_identifier_chars:
                i += 1
            
            
            if (source[i] == "'" and (i - start) == 1 and
                source[start:i] in 'uUL'):
                
                token_type = CONSTANT
                i = _GetChar(source, start, i)
            elif source[i] == "'" and source[start:i] in _STR_PREFIXES:
                token_type = CONSTANT
                i = _GetString(source, start, i)
        elif c == '/' and source[i+1] == '/':    
            i = source.find('\n', i)
            if i == -1:  
                i = end
            continue
        elif c == '/' and source[i+1] == '*':    
            i = source.find('*/', i) + 2
            continue
        elif c in ':+-<>&|*=':                   
            token_type = SYNTAX
            i += 1
            new_ch = source[i]
            if new_ch == c:
                i += 1
            elif c == '-' and new_ch == '>':
                i += 1
            elif new_ch == '=':
                i += 1
        elif c in '()[]{}~!?^%;/.,':             
            token_type = SYNTAX
            i += 1
            if c == '.' and source[i].isdigit():
                token_type = CONSTANT
                i += 1
                while source[i] in int_or_float_digits:
                    i += 1
                
                for suffix in ('l', 'f'):
                    if suffix == source[i:i+1].lower():
                        i += 1
                        break
        elif c.isdigit():                        
            token_type = CONSTANT
            if c == '0' and source[i+1] in 'xX':
                
                i += 2
                while source[i] in hex_digits:
                    i += 1
            else:
                while source[i] in int_or_float_digits2:
                    i += 1
            
            for suffix in ('ull', 'll', 'ul', 'l', 'f', 'u'):
                size = len(suffix)
                if suffix == source[i:i+size].lower():
                    i += size
                    break
        elif c == '"':                           
            token_type = CONSTANT
            i = _GetString(source, start, i)
        elif c == "'":                           
            token_type = CONSTANT
            i = _GetChar(source, start, i)
        elif c == '#':                           
            token_type = PREPROCESSOR
            got_if = source[i:i+3] == '#if' and source[i+3:i+4].isspace()
            if got_if:
                count_ifs += 1
            elif source[i:i+6] == '#endif':
                count_ifs -= 1
                if count_ifs == 0:
                    ignore_errors = False

            
            while 1:
                i1 = source.find('\n', i)
                i2 = source.find('//', i)
                i3 = source.find('/*', i)
                i4 = source.find('"', i)
                
                
                i = min([x for x in (i1, i2, i3, i4, end) if x != -1])

                
                if source[i] == '"':
                    i = source.find('"', i+1) + 1
                    assert i > 0
                    continue
                
                if not (i == i1 and source[i-1] == '\\'):
                    if got_if:
                        condition = source[start+4:i].lstrip()
                        if (condition.startswith('0') or
                            condition.startswith('(0)')):
                            ignore_errors = True
                    break
                i += 1
        elif c == '\\':                          
            
            i += 1
            continue
        elif ignore_errors:
            
            
            
            
            
            i += 1
        else:
            sys.stderr.write('Got invalid token in %s @ %d token:%s: %r\n' %
                             ('?', i, c, source[i-10:i+10]))
            raise RuntimeError('unexpected token')

        if i <= 0:
            print('Invalid index, exiting now.')
            return
        yield Token(token_type, source[start:i], start, i)


if __name__ == '__main__':
    def main(argv):
        """Driver mostly for testing purposes."""
        for filename in argv[1:]:
            source = utils.ReadFile(filename)
            if source is None:
                continue

            for token in GetTokens(source):
                print('%-12s: %s' % (token.token_type, token.name))
                
            sys.stdout.write('\n')


    main(sys.argv)
