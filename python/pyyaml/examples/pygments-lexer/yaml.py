
"""
yaml.py

Lexer for YAML, a human-friendly data serialization language
(http://yaml.org/).

Written by Kirill Simonov <xi@resolvent.net>.

License: Whatever suitable for inclusion into the Pygments package.
"""

from pygments.lexer import  \
        ExtendedRegexLexer, LexerContext, include, bygroups
from pygments.token import  \
        Text, Comment, Punctuation, Name, Literal

__all__ = ['YAMLLexer']


class YAMLLexerContext(LexerContext):
    """Indentation context for the YAML lexer."""

    def __init__(self, *args, **kwds):
        super(YAMLLexerContext, self).__init__(*args, **kwds)
        self.indent_stack = []
        self.indent = -1
        self.next_indent = 0
        self.block_scalar_indent = None


def something(TokenClass):
    """Do not produce empty tokens."""
    def callback(lexer, match, context):
        text = match.group()
        if not text:
            return
        yield match.start(), TokenClass, text
        context.pos = match.end()
    return callback

def reset_indent(TokenClass):
    """Reset the indentation levels."""
    def callback(lexer, match, context):
        text = match.group()
        context.indent_stack = []
        context.indent = -1
        context.next_indent = 0
        context.block_scalar_indent = None
        yield match.start(), TokenClass, text
        context.pos = match.end()
    return callback

def save_indent(TokenClass, start=False):
    """Save a possible indentation level."""
    def callback(lexer, match, context):
        text = match.group()
        extra = ''
        if start:
            context.next_indent = len(text)
            if context.next_indent < context.indent:
                while context.next_indent < context.indent:
                    context.indent = context.indent_stack.pop()
                if context.next_indent > context.indent:
                    extra = text[context.indent:]
                    text = text[:context.indent]
        else:
            context.next_indent += len(text)
        if text:
            yield match.start(), TokenClass, text
        if extra:
            yield match.start()+len(text), TokenClass.Error, extra
        context.pos = match.end()
    return callback

def set_indent(TokenClass, implicit=False):
    """Set the previously saved indentation level."""
    def callback(lexer, match, context):
        text = match.group()
        if context.indent < context.next_indent:
            context.indent_stack.append(context.indent)
            context.indent = context.next_indent
        if not implicit:
            context.next_indent += len(text)
        yield match.start(), TokenClass, text
        context.pos = match.end()
    return callback

def set_block_scalar_indent(TokenClass):
    """Set an explicit indentation level for a block scalar."""
    def callback(lexer, match, context):
        text = match.group()
        context.block_scalar_indent = None
        if not text:
            return
        increment = match.group(1)
        if increment:
            current_indent = max(context.indent, 0)
            increment = int(increment)
            context.block_scalar_indent = current_indent + increment
        if text:
            yield match.start(), TokenClass, text
            context.pos = match.end()
    return callback

def parse_block_scalar_empty_line(IndentTokenClass, ContentTokenClass):
    """Process an empty line in a block scalar."""
    def callback(lexer, match, context):
        text = match.group()
        if (context.block_scalar_indent is None or
                len(text) <= context.block_scalar_indent):
            if text:
                yield match.start(), IndentTokenClass, text
        else:
            indentation = text[:context.block_scalar_indent]
            content = text[context.block_scalar_indent:]
            yield match.start(), IndentTokenClass, indentation
            yield (match.start()+context.block_scalar_indent,
                    ContentTokenClass, content)
        context.pos = match.end()
    return callback

def parse_block_scalar_indent(TokenClass):
    """Process indentation spaces in a block scalar."""
    def callback(lexer, match, context):
        text = match.group()
        if context.block_scalar_indent is None:
            if len(text) <= max(context.indent, 0):
                context.stack.pop()
                context.stack.pop()
                return
            context.block_scalar_indent = len(text)
        else:
            if len(text) < context.block_scalar_indent:
                context.stack.pop()
                context.stack.pop()
                return
        if text:
            yield match.start(), TokenClass, text
            context.pos = match.end()
    return callback

def parse_plain_scalar_indent(TokenClass):
    """Process indentation spaces in a plain scalar."""
    def callback(lexer, match, context):
        text = match.group()
        if len(text) <= context.indent:
            context.stack.pop()
            context.stack.pop()
            return
        if text:
            yield match.start(), TokenClass, text
            context.pos = match.end()
    return callback


class YAMLLexer(ExtendedRegexLexer):
    """Lexer for the YAML language."""

    name = 'YAML'
    aliases = ['yaml']
    filenames = ['*.yaml', '*.yml']
    mimetypes = ['text/x-yaml']

    tokens = {

        
        'root': [
            
            (r'[ ]+(?=#|$)', Text.Blank),
            
            (r'\n+', Text.Break),
            
            (r'#[^\n]*', Comment.Single),
            
            (r'^%YAML(?=[ ]|$)', reset_indent(Name.Directive),
                'yaml-directive'),
            
            (r'^%TAG(?=[ ]|$)', reset_indent(Name.Directive),
                'tag-directive'),
            
            (r'^(?:---|\.\.\.)(?=[ ]|$)',
                reset_indent(Punctuation.Document), 'block-line'),
            
            (r'[ ]*(?![ \t\n\r\f\v]|$)',
                save_indent(Text.Indent, start=True),
                ('block-line', 'indentation')),
        ],

        
        'ignored-line': [
            
            (r'[ ]+(?=#|$)', Text.Blank),
            
            (r'#[^\n]*', Comment.Single),
            
            (r'\n', Text.Break, '#pop:2'),
        ],

        
        'yaml-directive': [
            
            (r'([ ]+)([0-9]+\.[0-9]+)',
                bygroups(Text.Blank, Literal.Version), 'ignored-line'),
        ],

        
        'tag-directive': [
            
            (r'([ ]+)(!|![0-9A-Za-z_-]*!)'
                r'([ ]+)(!|!?[0-9A-Za-z;/?:@&=+$,_.!~*\'()\[\]%-]+)',
                bygroups(Text.Blank, Name.Type, Text.Blank, Name.Type),
                'ignored-line'),
        ],

        
        'indentation': [
            
            (r'[ ]*$', something(Text.Blank), '#pop:2'),
            
            (r'[ ]+(?=[?:-](?:[ ]|$))', save_indent(Text.Indent)),
            
            (r'[?:-](?=[ ]|$)', set_indent(Punctuation.Indicator)),
            
            (r'[ ]*', save_indent(Text.Indent), '#pop'),
        ],

        
        'block-line': [
            
            (r'[ ]*(?=#|$)', something(Text.Blank), '#pop'),
            
            (r'[ ]+', Text.Blank),
            
            include('descriptors'),
            
            include('block-nodes'),
            
            include('flow-nodes'),
            
            (r'(?=[^ \t\n\r\f\v?:,\[\]{}#&*!|>\'"%@`-]|[?:-][^ \t\n\r\f\v])',
                something(Literal.Scalar.Plain),
                'plain-scalar-in-block-context'),
        ],

        
        'descriptors' : [
            
            (r'!<[0-9A-Za-z;/?:@&=+$,_.!~*\'()\[\]%-]+>', Name.Type),
            
            (r'!(?:[0-9A-Za-z_-]+)?'
                r'(?:![0-9A-Za-z;/?:@&=+$,_.!~*\'()\[\]%-]+)?', Name.Type),
            
            (r'&[0-9A-Za-z_-]+', Name.Anchor),
            
            (r'\*[0-9A-Za-z_-]+', Name.Alias),
        ],

        
        'block-nodes': [
            
            (r':(?=[ ]|$)', set_indent(Punctuation.Indicator, implicit=True)),
            
            (r'[|>]', Punctuation.Indicator,
                ('block-scalar-content', 'block-scalar-header')),
        ],

        
        'flow-nodes': [
            
            (r'\[', Punctuation.Indicator, 'flow-sequence'),
            
            (r'\{', Punctuation.Indicator, 'flow-mapping'),
            
            (r'\'', Literal.Scalar.Flow.Quote, 'single-quoted-scalar'),
            
            (r'\"', Literal.Scalar.Flow.Quote, 'double-quoted-scalar'),
        ],

        
        'flow-collection': [
            
            (r'[ ]+', Text.Blank),
            
            (r'\n+', Text.Break),
            
            (r'#[^\n]*', Comment.Single),
            
            (r'[?:,]', Punctuation.Indicator),
            
            include('descriptors'),
            
            include('flow-nodes'),
            
            (r'(?=[^ \t\n\r\f\v?:,\[\]{}#&*!|>\'"%@`])',
                something(Literal.Scalar.Plain),
                'plain-scalar-in-flow-context'),
        ],

        
        'flow-sequence': [
            
            include('flow-collection'),
            
            (r'\]', Punctuation.Indicator, '#pop'),
        ],

        
        'flow-mapping': [
            
            include('flow-collection'),
            
            (r'\}', Punctuation.Indicator, '#pop'),
        ],

        
        'block-scalar-content': [
            
            (r'\n', Text.Break),
            
            (r'^[ ]+$',
                parse_block_scalar_empty_line(Text.Indent,
                    Literal.Scalar.Block)),
            
            (r'^[ ]*', parse_block_scalar_indent(Text.Indent)),
            
            (r'[^\n\r\f\v]+', Literal.Scalar.Block),
        ],

        
        'block-scalar-header': [
            
            (r'([1-9])?[+-]?(?=[ ]|$)',
                set_block_scalar_indent(Punctuation.Indicator),
                'ignored-line'),
            
            (r'[+-]?([1-9])?(?=[ ]|$)',
                set_block_scalar_indent(Punctuation.Indicator),
                'ignored-line'),
        ],

        
        'quoted-scalar-whitespaces': [
            
            (r'^[ ]+|[ ]+$', Text.Blank),
            
            (r'\n+', Text.Break),
            
            (r'[ ]+', Literal.Scalar.Flow),
        ],

        
        'single-quoted-scalar': [
            
            include('quoted-scalar-whitespaces'),
            
            (r'\'\'', Literal.Scalar.Flow.Escape),
            
            (r'[^ \t\n\r\f\v\']+', Literal.Scalar.Flow),
            
            (r'\'', Literal.Scalar.Flow.Quote, '#pop'),
        ],

        
        'double-quoted-scalar': [
            
            include('quoted-scalar-whitespaces'),
            
            (r'\\[0abt\tn\nvfre "\\N_LP]', Literal.Scalar.Flow.Escape),
            
            (r'\\(?:x[0-9A-Fa-f]{2}|u[0-9A-Fa-f]{4}|U[0-9A-Fa-f]{8})',
                Literal.Scalar.Flow.Escape),
            
            (r'[^ \t\n\r\f\v\"\\]+', Literal.Scalar.Flow),
            
            (r'"', Literal.Scalar.Flow.Quote, '#pop'),
        ],

        
        'plain-scalar-in-block-context-new-line': [
            
            (r'^[ ]+$', Text.Blank),
            
            (r'\n+', Text.Break),
            
            (r'^(?=---|\.\.\.)', something(Punctuation.Document), '#pop:3'),
            
            (r'^[ ]*', parse_plain_scalar_indent(Text.Indent), '#pop'),
        ],

        
        'plain-scalar-in-block-context': [
            
            (r'[ ]*(?=:[ ]|:$)', something(Text.Blank), '#pop'),
            
            (r'[ ]+(?=#)', Text.Blank, '#pop'),
            
            (r'[ ]+$', Text.Blank),
            
            (r'\n+', Text.Break, 'plain-scalar-in-block-context-new-line'),
            
            (r'[ ]+', Literal.Scalar.Plain),
            
            (r'(?::(?![ \t\n\r\f\v])|[^ \t\n\r\f\v:])+',
                Literal.Scalar.Plain),
        ],

        
        'plain-scalar-in-flow-context': [
            
            (r'[ ]*(?=[,:?\[\]{}])', something(Text.Blank), '#pop'),
            
            (r'[ ]+(?=#)', Text.Blank, '#pop'),
            
            (r'^[ ]+|[ ]+$', Text.Blank),
            
            (r'\n+', Text.Break),
            
            (r'[ ]+', Literal.Scalar.Plain),
            
            (r'[^ \t\n\r\f\v,:?\[\]{}]+', Literal.Scalar.Plain),
        ],

    }

    def get_tokens_unprocessed(self, text=None, context=None):
        if context is None:
            context = YAMLLexerContext(text, 0)
        return super(YAMLLexer, self).get_tokens_unprocessed(text, context)


