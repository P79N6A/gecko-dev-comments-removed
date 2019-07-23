




















import fileinput, re, sys, os, string














class  SourceBlockFormat:

    def  __init__( self, id, start, column, end ):
        """create a block pattern, used to recognize special documentation blocks"""
        self.id     = id
        self.start  = re.compile( start, re.VERBOSE )
        self.column = re.compile( column, re.VERBOSE )
        self.end    = re.compile( end, re.VERBOSE )















start = r'''
  \s*      # any number of whitespace
  /\*{2,}/ # followed by '/' and at least two asterisks then '/'
  \s*$     # probably followed by whitespace
'''

column = r'''
  \s*      # any number of whitespace
  /\*{1}   # followed by '/' and precisely one asterisk
  ([^*].*) # followed by anything (group 1)
  \*{1}/   # followed by one asterisk and a '/'
  \s*$     # probably followed by whitespace
'''

re_source_block_format1 = SourceBlockFormat( 1, start, column, start )














start = r'''
  \s*     # any number of whitespace
  /\*{2,} # followed by '/' and at least two asterisks
  \s*$    # probably followed by whitespace
'''

column = r'''
  \s*        # any number of whitespace
  \*{1}(?!/) # followed by precisely one asterisk not followed by `/'
  (.*)       # then anything (group1)
'''

end = r'''
  \s*  # any number of whitespace
  \*+/ # followed by at least one asterisk, then '/'
'''

re_source_block_format2 = SourceBlockFormat( 2, start, column, end )






re_source_block_formats = [re_source_block_format1, re_source_block_format2]









re_markup_tag1 = re.compile( r'''\s*<(\w*)>''' )  
re_markup_tag2 = re.compile( r'''\s*@(\w*):''' )  





re_markup_tags = [re_markup_tag1, re_markup_tag2]




re_crossref = re.compile( r'@(\w*)(.*)' )




re_italic = re.compile( r"_(\w(\w|')*)_(.*)" )     
re_bold   = re.compile( r"\*(\w(\w|')*)\*(.*)" )   




re_source_sep = re.compile( r'\s*/\*\s*\*/' )




re_source_crossref = re.compile( r'(\W*)(\w*)' )




re_source_keywords = re.compile( '''\\b ( typedef   |
                                          struct    |
                                          enum      |
                                          union     |
                                          const     |
                                          char      |
                                          int       |
                                          short     |
                                          long      |
                                          void      |
                                          signed    |
                                          unsigned  |
                                          \#include |
                                          \#define  |
                                          \#undef   |
                                          \#if      |
                                          \#ifdef   |
                                          \#ifndef  |
                                          \#else    |
                                          \#endif   ) \\b''', re.VERBOSE )

























class  SourceBlock:

    def  __init__( self, processor, filename, lineno, lines ):
        self.processor = processor
        self.filename  = filename
        self.lineno    = lineno
        self.lines     = lines[:]
        self.format    = processor.format
        self.content   = []

        if self.format == None:
            return

        words = []

        
        lines = []

        for line0 in self.lines:
            m = self.format.column.match( line0 )
            if m:
                lines.append( m.group( 1 ) )

        
        for l in lines:
            l = string.strip( l )
            if len( l ) > 0:
                for tag in re_markup_tags:
                    if tag.match( l ):
                        self.content = lines
                        return

    def  location( self ):
        return "(" + self.filename + ":" + repr( self.lineno ) + ")"

    
    def  dump( self ):
        if self.content:
            print "{{{content start---"
            for l in self.content:
                print l
            print "---content end}}}"
            return

        fmt = ""
        if self.format:
            fmt = repr( self.format.id ) + " "

        for line in self.lines:
            print line



















class  SourceProcessor:

    def  __init__( self ):
        """initialize a source processor"""
        self.blocks   = []
        self.filename = None
        self.format   = None
        self.lines    = []

    def  reset( self ):
        """reset a block processor, clean all its blocks"""
        self.blocks = []
        self.format = None

    def  parse_file( self, filename ):
        """parse a C source file, and add its blocks to the processor's list"""
        self.reset()

        self.filename = filename

        fileinput.close()
        self.format = None
        self.lineno = 0
        self.lines  = []

        for line in fileinput.input( filename ):
            
            if line[-1] == '\012':
                line = line[0:-1]

            if self.format == None:
                self.process_normal_line( line )
            else:
                if self.format.end.match( line ):
                    
                    
                    self.lines.append( line )
                    self.add_block_lines()
                elif self.format.column.match( line ):
                    
                    self.lines.append( line )
                else:
                    
                    
                    self.add_block_lines()

                    
                    self.process_normal_line( line )

        
        self.add_block_lines()

    def  process_normal_line( self, line ):
        """process a normal line and check whether it is the start of a new block"""
        for f in re_source_block_formats:
            if f.start.match( line ):
                self.add_block_lines()
                self.format = f
                self.lineno = fileinput.filelineno()

        self.lines.append( line )

    def  add_block_lines( self ):
        """add the current accumulated lines and create a new block"""
        if self.lines != []:
            block = SourceBlock( self, self.filename, self.lineno, self.lines )

            self.blocks.append( block )
            self.format = None
            self.lines  = []

    
    def  dump( self ):
        """print all blocks in a processor"""
        for b in self.blocks:
            b.dump()


