"""
CORE MARKDOWN BLOCKPARSER
=============================================================================

This parser handles basic parsing of Markdown blocks.  It doesn't concern itself
with inline elements such as **bold** or *italics*, but rather just catches 
blocks, lists, quotes, etc.

The BlockParser is made up of a bunch of BlockProssors, each handling a 
different type of block. Extensions may add/replace/remove BlockProcessors
as they need to alter how markdown blocks are parsed.

"""

import re
import markdown

class BlockProcessor:
    """ Base class for block processors. 
    
    Each subclass will provide the methods below to work with the source and
    tree. Each processor will need to define it's own ``test`` and ``run``
    methods. The ``test`` method should return True or False, to indicate
    whether the current block should be processed by this processor. If the
    test passes, the parser will call the processors ``run`` method.

    """

    def __init__(self, parser=None):
        self.parser = parser

    def lastChild(self, parent):
        """ Return the last child of an etree element. """
        if len(parent):
            return parent[-1]
        else:
            return None

    def detab(self, text):
        """ Remove a tab from the front of each line of the given text. """
        newtext = []
        lines = text.split('\n')
        for line in lines:
            if line.startswith(' '*markdown.TAB_LENGTH):
                newtext.append(line[markdown.TAB_LENGTH:])
            elif not line.strip():
                newtext.append('')
            else:
                break
        return '\n'.join(newtext), '\n'.join(lines[len(newtext):])

    def looseDetab(self, text, level=1):
        """ Remove a tab from front of lines but allowing dedented lines. """
        lines = text.split('\n')
        for i in range(len(lines)):
            if lines[i].startswith(' '*markdown.TAB_LENGTH*level):
                lines[i] = lines[i][markdown.TAB_LENGTH*level:]
        return '\n'.join(lines)

    def test(self, parent, block):
        """ Test for block type. Must be overridden by subclasses. 
        
        As the parser loops through processors, it will call the ``test`` method
        on each to determine if the given block of text is of that type. This
        method must return a boolean ``True`` or ``False``. The actual method of
        testing is left to the needs of that particular block type. It could 
        be as simple as ``block.startswith(some_string)`` or a complex regular
        expression. As the block type may be different depending on the parent
        of the block (i.e. inside a list), the parent etree element is also 
        provided and may be used as part of the test.

        Keywords:
        
        * ``parent``: A etree element which will be the parent of the block.
        * ``block``: A block of text from the source which has been split at 
            blank lines.
        """
        pass

    def run(self, parent, blocks):
        """ Run processor. Must be overridden by subclasses. 
        
        When the parser determines the appropriate type of a block, the parser
        will call the corresponding processor's ``run`` method. This method
        should parse the individual lines of the block and append them to
        the etree. 

        Note that both the ``parent`` and ``etree`` keywords are pointers
        to instances of the objects which should be edited in place. Each
        processor must make changes to the existing objects as there is no
        mechanism to return new/different objects to replace them.

        This means that this method should be adding SubElements or adding text
        to the parent, and should remove (``pop``) or add (``insert``) items to
        the list of blocks.

        Keywords:

        * ``parent``: A etree element which is the parent of the current block.
        * ``blocks``: A list of all remaining blocks of the document.
        """
        pass


class ListIndentProcessor(BlockProcessor):
    """ Process children of list items. 
    
    Example:
        * a list item
            process this part

            or this part

    """

    INDENT_RE = re.compile(r'^(([ ]{%s})+)'% markdown.TAB_LENGTH)
    ITEM_TYPES = ['li']
    LIST_TYPES = ['ul', 'ol']

    def test(self, parent, block):
        return block.startswith(' '*markdown.TAB_LENGTH) and \
                not self.parser.state.isstate('detabbed') and  \
                (parent.tag in self.ITEM_TYPES or \
                    (len(parent) and parent[-1] and \
                        (parent[-1].tag in self.LIST_TYPES)
                    )
                )

    def run(self, parent, blocks):
        block = blocks.pop(0)
        level, sibling = self.get_level(parent, block)
        block = self.looseDetab(block, level)

        self.parser.state.set('detabbed')
        if parent.tag in self.ITEM_TYPES:
            
            self.parser.parseBlocks(parent, [block])
        elif sibling.tag in self.ITEM_TYPES:
            
            self.parser.parseBlocks(sibling, [block])
        elif len(sibling) and sibling[-1].tag in self.ITEM_TYPES:
            
            
            if sibling[-1].text:
                
                block = '%s\n\n%s' % (sibling[-1].text, block)
                sibling[-1].text = ''
            self.parser.parseChunk(sibling[-1], block)
        else:
            self.create_item(sibling, block)
        self.parser.state.reset()

    def create_item(self, parent, block):
        """ Create a new li and parse the block with it as the parent. """
        li = markdown.etree.SubElement(parent, 'li')
        self.parser.parseBlocks(li, [block])
 
    def get_level(self, parent, block):
        """ Get level of indent based on list level. """
        
        m = self.INDENT_RE.match(block)
        if m:
            indent_level = len(m.group(1))/markdown.TAB_LENGTH
        else:
            indent_level = 0
        if self.parser.state.isstate('list'):
            
            level = 1
        else:
            
            level = 0
        
        while indent_level > level:
            child = self.lastChild(parent)
            if child and (child.tag in self.LIST_TYPES or child.tag in self.ITEM_TYPES):
                if child.tag in self.LIST_TYPES:
                    level += 1
                parent = child
            else:
                
                
                break
        return level, parent


class CodeBlockProcessor(BlockProcessor):
    """ Process code blocks. """

    def test(self, parent, block):
        return block.startswith(' '*markdown.TAB_LENGTH)
    
    def run(self, parent, blocks):
        sibling = self.lastChild(parent)
        block = blocks.pop(0)
        theRest = ''
        if sibling and sibling.tag == "pre" and len(sibling) \
                    and sibling[0].tag == "code":
            
            
            
            code = sibling[0]
            block, theRest = self.detab(block)
            code.text = markdown.AtomicString('%s\n%s\n' % (code.text, block.rstrip()))
        else:
            
            pre = markdown.etree.SubElement(parent, 'pre')
            code = markdown.etree.SubElement(pre, 'code')
            block, theRest = self.detab(block)
            code.text = markdown.AtomicString('%s\n' % block.rstrip())
        if theRest:
            
            
            
            blocks.insert(0, theRest)


class BlockQuoteProcessor(BlockProcessor):

    RE = re.compile(r'(^|\n)[ ]{0,3}>[ ]?(.*)')

    def test(self, parent, block):
        return bool(self.RE.search(block))

    def run(self, parent, blocks):
        block = blocks.pop(0)
        m = self.RE.search(block)
        if m:
            before = block[:m.start()] 
            
            self.parser.parseBlocks(parent, [before])
            
            block = '\n'.join([self.clean(line) for line in 
                            block[m.start():].split('\n')])
        sibling = self.lastChild(parent)
        if sibling and sibling.tag == "blockquote":
            
            quote = sibling
        else:
            
            quote = markdown.etree.SubElement(parent, 'blockquote')
        
        self.parser.parseChunk(quote, block)

    def clean(self, line):
        """ Remove ``>`` from beginning of a line. """
        m = self.RE.match(line)
        if line.strip() == ">":
            return ""
        elif m:
            return m.group(2)
        else:
            return line

class OListProcessor(BlockProcessor):
    """ Process ordered list blocks. """

    TAG = 'ol'
    
    RE = re.compile(r'^[ ]{0,3}\d+\.[ ](.*)')
    
    CHILD_RE = re.compile(r'^[ ]{0,3}((\d+\.)|[*+-])[ ](.*)')
    
    INDENT_RE = re.compile(r'^[ ]{4,7}((\d+\.)|[*+-])[ ].*')

    def test(self, parent, block):
        return bool(self.RE.match(block))

    def run(self, parent, blocks):
        
        items = self.get_items(blocks.pop(0))
        sibling = self.lastChild(parent)
        if sibling and sibling.tag in ['ol', 'ul']:
            
            lst = sibling
            
            if len(lst) and lst[-1].text and not len(lst[-1]):
                p = markdown.etree.SubElement(lst[-1], 'p')
                p.text = lst[-1].text
                lst[-1].text = ''
            
            li = markdown.etree.SubElement(lst, 'li')
            self.parser.state.set('looselist')
            firstitem = items.pop(0)
            self.parser.parseBlocks(li, [firstitem])
            self.parser.state.reset()
        else:
            
            lst = markdown.etree.SubElement(parent, self.TAG)
        self.parser.state.set('list')
        
        
        for item in items:
            if item.startswith(' '*markdown.TAB_LENGTH):
                
                self.parser.parseBlocks(lst[-1], [item])
            else:
                
                li = markdown.etree.SubElement(lst, 'li')
                self.parser.parseBlocks(li, [item])
        self.parser.state.reset()

    def get_items(self, block):
        """ Break a block into list items. """
        items = []
        for line in block.split('\n'):
            m = self.CHILD_RE.match(line)
            if m:
                
                items.append(m.group(3))
            elif self.INDENT_RE.match(line):
                
                if items[-1].startswith(' '*markdown.TAB_LENGTH):
                    
                    items[-1] = '%s\n%s' % (items[-1], line)
                else:
                    items.append(line)
            else:
                
                items[-1] = '%s\n%s' % (items[-1], line)
        return items


class UListProcessor(OListProcessor):
    """ Process unordered list blocks. """

    TAG = 'ul'
    RE = re.compile(r'^[ ]{0,3}[*+-][ ](.*)')


class HashHeaderProcessor(BlockProcessor):
    """ Process Hash Headers. """

    
    RE = re.compile(r'(^|\n)(?P<level>#{1,6})(?P<header>.*?)#*(\n|$)')

    def test(self, parent, block):
        return bool(self.RE.search(block))

    def run(self, parent, blocks):
        block = blocks.pop(0)
        m = self.RE.search(block)
        if m:
            before = block[:m.start()] 
            after = block[m.end():]    
            if before:
                
                
                
                self.parser.parseBlocks(parent, [before])
            
            h = markdown.etree.SubElement(parent, 'h%d' % len(m.group('level')))
            h.text = m.group('header').strip()
            if after:
                
                blocks.insert(0, after)
        else:
            
            message(CRITICAL, "We've got a problem header!")


class SetextHeaderProcessor(BlockProcessor):
    """ Process Setext-style Headers. """

    
    RE = re.compile(r'^.*?\n[=-]{3,}', re.MULTILINE)

    def test(self, parent, block):
        return bool(self.RE.match(block))

    def run(self, parent, blocks):
        lines = blocks.pop(0).split('\n')
        
        if lines[1].startswith('='):
            level = 1
        else:
            level = 2
        h = markdown.etree.SubElement(parent, 'h%d' % level)
        h.text = lines[0].strip()
        if len(lines) > 2:
            
            blocks.insert(0, '\n'.join(lines[2:]))


class HRProcessor(BlockProcessor):
    """ Process Horizontal Rules. """

    RE = r'[ ]{0,3}(?P<ch>[*_-])[ ]?((?P=ch)[ ]?){2,}[ ]*'
    
    SEARCH_RE = re.compile(r'(^|\n)%s(\n|$)' % RE)
    
    MATCH_RE = re.compile(r'^%s$' % RE)

    def test(self, parent, block):
        return bool(self.SEARCH_RE.search(block))

    def run(self, parent, blocks):
        lines = blocks.pop(0).split('\n')
        prelines = []
        
        for line in lines:
            m = self.MATCH_RE.match(line)
            if m:
                break
            else:
                prelines.append(line)
        if len(prelines):
            
            self.parser.parseBlocks(parent, ['\n'.join(prelines)])
        
        hr = markdown.etree.SubElement(parent, 'hr')
        
        lines = lines[len(prelines)+1:]
        if len(lines):
            
            blocks.insert(0, '\n'.join(lines))


class EmptyBlockProcessor(BlockProcessor):
    """ Process blocks and start with an empty line. """

    
    
    RE = re.compile(r'^\s*\n')

    def test(self, parent, block):
        return bool(self.RE.match(block))

    def run(self, parent, blocks):
        block = blocks.pop(0)
        m = self.RE.match(block)
        if m:
            
            blocks.insert(0, block[m.end():])
            sibling = self.lastChild(parent)
            if sibling and sibling.tag == 'pre' and sibling[0] and \
                    sibling[0].tag == 'code':
                
                sibling[0].text = markdown.AtomicString('%s/n/n/n' % sibling[0].text )


class ParagraphProcessor(BlockProcessor):
    """ Process Paragraph blocks. """

    def test(self, parent, block):
        return True

    def run(self, parent, blocks):
        block = blocks.pop(0)
        if block.strip():
            
            if self.parser.state.isstate('list'):
                
                if parent.text:
                    parent.text = '%s\n%s' % (parent.text, block)
                else:
                    parent.text = block.lstrip()
            else:
                
                p = markdown.etree.SubElement(parent, 'p')
                p.text = block.lstrip()
