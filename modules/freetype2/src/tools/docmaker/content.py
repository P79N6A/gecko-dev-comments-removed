





from sources import *
from utils import *
import string, re

















re_code_start = re.compile( r"(\s*){\s*$" )
re_code_end   = re.compile( r"(\s*)}\s*$" )





re_identifier = re.compile( r'(\w*)' )






re_header_macro = re.compile( r'^#define\s{1,}(\w{1,}_H)\s{1,}<(.*)>' )












class  DocCode:

    def  __init__( self, margin, lines ):
        self.lines = []
        self.words = None

        
        for l in lines:
            if string.strip( l[:margin] ) == "":
                l = l[margin:]
            self.lines.append( l )

    def  dump( self, prefix = "", width = 60 ):
        lines = self.dump_lines( 0, width )
        for l in lines:
            print prefix + l

    def  dump_lines( self, margin = 0, width = 60 ):
        result = []
        for l in self.lines:
            result.append( " " * margin + l )
        return result









class  DocPara:

    def  __init__( self, lines ):
        self.lines = None
        self.words = []
        for l in lines:
            l = string.strip( l )
            self.words.extend( string.split( l ) )

    def  dump( self, prefix = "", width = 60 ):
        lines = self.dump_lines( 0, width )
        for l in lines:
            print prefix + l

    def  dump_lines( self, margin = 0, width = 60 ):
        cur    = ""  
        col    = 0   
        result = []

        for word in self.words:
            ln = len( word )
            if col > 0:
                ln = ln + 1

            if col + ln > width:
                result.append( " " * margin + cur )
                cur = word
                col = len( word )
            else:
                if col > 0:
                    cur = cur + " "
                cur = cur + word
                col = col + ln

        if col > 0:
            result.append( " " * margin + cur )

        return result









class  DocField:

    def  __init__( self, name, lines ):
        self.name  = name  
        self.items = []    

        mode_none  = 0     
        mode_code  = 1     
        mode_para  = 3     

        margin     = -1    
        cur_lines  = []

        
        
        
        start = 0
        mode  = mode_none

        for l in lines:
            
            if mode == mode_code:
                m = re_code_end.match( l )
                if m and len( m.group( 1 ) ) <= margin:
                    
                    code = DocCode( 0, cur_lines )
                    self.items.append( code )
                    margin    = -1
                    cur_lines = []
                    mode      = mode_none
                else:
                    
                    cur_lines.append( l[margin:] )
            else:
                
                m = re_code_start.match( l )
                if m:
                    
                    if cur_lines:
                        para = DocPara( cur_lines )
                        self.items.append( para )
                        cur_lines = []

                    
                    margin = len( m.group( 1 ) )
                    mode   = mode_code
                else:
                    if not string.split( l ) and cur_lines:
                        
                        
                        para = DocPara( cur_lines )
                        self.items.append( para )
                        cur_lines = []
                    else:
                        
                        
                        cur_lines.append( l )

        if mode == mode_code:
            
            code = DocCode( margin, cur_lines )
            self.items.append( code )
        elif cur_lines:
            para = DocPara( cur_lines )
            self.items.append( para )

    def  dump( self, prefix = "" ):
        if self.field:
            print prefix + self.field + " ::"
            prefix = prefix + "----"

        first = 1
        for p in self.items:
            if not first:
                print ""
            p.dump( prefix )
            first = 0

    def  dump_lines( self, margin = 0, width = 60 ):
        result = []
        nl     = None

        for p in self.items:
            if nl:
                result.append( "" )

            result.extend( p.dump_lines( margin, width ) )
            nl = 1

        return result





re_field = re.compile( r"\s*(\w*|\w(\w|\.)*\w)\s*::" )



class  DocMarkup:

    def  __init__( self, tag, lines ):
        self.tag    = string.lower( tag )
        self.fields = []

        cur_lines = []
        field     = None
        mode      = 0

        for l in lines:
            m = re_field.match( l )
            if m:
                

                
                if cur_lines:
                    f = DocField( field, cur_lines )
                    self.fields.append( f )
                    cur_lines = []
                    field     = None

                field     = m.group( 1 )   
                ln        = len( m.group( 0 ) )
                l         = " " * ln + l[ln:]
                cur_lines = [l]
            else:
                cur_lines.append( l )

        if field or cur_lines:
            f = DocField( field, cur_lines )
            self.fields.append( f )

    def  get_name( self ):
        try:
            return self.fields[0].items[0].words[0]
        except:
            return None

    def  get_start( self ):
        try:
            result = ""
            for word in self.fields[0].items[0].words:
                result = result + " " + word
            return result[1:]
        except:
            return "ERROR"

    def  dump( self, margin ):
        print " " * margin + "<" + self.tag + ">"
        for f in self.fields:
            f.dump( "  " )
        print " " * margin + "</" + self.tag + ">"



class  DocChapter:

    def  __init__( self, block ):
        self.block    = block
        self.sections = []
        if block:
            self.name  = block.name
            self.title = block.get_markup_words( "title" )
            self.order = block.get_markup_words( "sections" )
        else:
            self.name  = "Other"
            self.title = string.split( "Miscellaneous" )
            self.order = []



class  DocSection:

    def  __init__( self, name = "Other" ):
        self.name        = name
        self.blocks      = {}
        self.block_names = []  
        self.defs        = []
        self.abstract    = ""
        self.description = ""
        self.order       = []
        self.title       = "ERROR"
        self.chapter     = None

    def  add_def( self, block ):
        self.defs.append( block )

    def  add_block( self, block ):
        self.block_names.append( block.name )
        self.blocks[block.name] = block

    def  process( self ):
        
        for block in self.defs:
            title = block.get_markup_text( "title" )
            if title:
                self.title       = title
                self.abstract    = block.get_markup_words( "abstract" )
                self.description = block.get_markup_items( "description" )
                self.order       = block.get_markup_words( "order" )
                return

    def  reorder( self ):
        self.block_names = sort_order_list( self.block_names, self.order )



class  ContentProcessor:

    def  __init__( self ):
        """initialize a block content processor"""
        self.reset()

        self.sections = {}    
        self.section  = None  

        self.chapters = []    

        self.headers  = {}    

    def  set_section( self, section_name ):
        """set current section during parsing"""
        if not self.sections.has_key( section_name ):
            section = DocSection( section_name )
            self.sections[section_name] = section
            self.section                = section
        else:
            self.section = self.sections[section_name]

    def  add_chapter( self, block ):
        chapter = DocChapter( block )
        self.chapters.append( chapter )


    def  reset( self ):
        """reset the content processor for a new block"""
        self.markups      = []
        self.markup       = None
        self.markup_lines = []

    def  add_markup( self ):
        """add a new markup section"""
        if self.markup and self.markup_lines:

            
            marks = self.markup_lines
            if len( marks ) > 0 and not string.strip( marks[-1] ):
                self.markup_lines = marks[:-1]

            m = DocMarkup( self.markup, self.markup_lines )

            self.markups.append( m )

            self.markup       = None
            self.markup_lines = []

    def  process_content( self, content ):
        """process a block content and return a list of DocMarkup objects
           corresponding to it"""
        markup       = None
        markup_lines = []
        first        = 1

        for line in content:
            found = None
            for t in re_markup_tags:
                m = t.match( line )
                if m:
                    found  = string.lower( m.group( 1 ) )
                    prefix = len( m.group( 0 ) )
                    line   = " " * prefix + line[prefix:]   
                    break

            
            if found:
                first = 0
                self.add_markup()  
                self.markup = found
                if len( string.strip( line ) ) > 0:
                    self.markup_lines.append( line )
            elif first == 0:
                self.markup_lines.append( line )

        self.add_markup()

        return self.markups

    def  parse_sources( self, source_processor ):
        blocks = source_processor.blocks
        count  = len( blocks )

        for n in range( count ):
            source = blocks[n]
            if source.content:
                
                
                
                follow = []
                m = n + 1
                while m < count and not blocks[m].content:
                    follow.append( blocks[m] )
                    m = m + 1

                doc_block = DocBlock( source, follow, self )

    def  finish( self ):
        
        
        
        for sec in self.sections.values():
            sec.process()

        
        
        for chap in self.chapters:
            for sec in chap.order:
                if self.sections.has_key( sec ):
                    section = self.sections[sec]
                    section.chapter = chap
                    section.reorder()
                    chap.sections.append( section )
                else:
                    sys.stderr.write( "WARNING: chapter '" +          \
                        chap.name + "' in " + chap.block.location() + \
                        " lists unknown section '" + sec + "'\n" )

        
        
        others = []
        for sec in self.sections.values():
            if not sec.chapter:
                others.append( sec )

        
        
        
        if others:
            chap = DocChapter( None )
            chap.sections = others
            self.chapters.append( chap )



class  DocBlock:

    def  __init__( self, source, follow, processor ):
        processor.reset()

        self.source  = source
        self.code    = []
        self.type    = "ERRTYPE"
        self.name    = "ERRNAME"
        self.section = processor.section
        self.markups = processor.process_content( source.content )

        
        try:
            self.type = self.markups[0].tag
        except:
            pass

        
        try:
            markup = self.markups[0]
            para   = markup.fields[0].items[0]
            name   = para.words[0]
            m = re_identifier.match( name )
            if m:
                name = m.group( 1 )
            self.name = name
        except:
            pass

        if self.type == "section":
            
            processor.set_section( self.name )
            processor.section.add_def( self )
        elif self.type == "chapter":
            
            processor.add_chapter( self )
        else:
            processor.section.add_block( self )

        
        
        source = []
        for b in follow:
            if b.format:
                break
            for l in b.lines:
                
                m = re_header_macro.match( l )
                if m:
                    processor.headers[m.group( 2 )] = m.group( 1 );

                
                if re_source_sep.match( l ):
                    break
                source.append( l )

        
        start = 0
        end   = len( source ) - 1

        while start < end and not string.strip( source[start] ):
            start = start + 1

        while start < end and not string.strip( source[end] ):
            end = end - 1

        source = source[start:end + 1]

        self.code = source

    def  location( self ):
        return self.source.location()

    def  get_markup( self, tag_name ):
        """return the DocMarkup corresponding to a given tag in a block"""
        for m in self.markups:
            if m.tag == string.lower( tag_name ):
                return m
        return None

    def  get_markup_name( self, tag_name ):
        """return the name of a given primary markup in a block"""
        try:
            m = self.get_markup( tag_name )
            return m.get_name()
        except:
            return None

    def  get_markup_words( self, tag_name ):
        try:
            m = self.get_markup( tag_name )
            return m.fields[0].items[0].words
        except:
            return []

    def  get_markup_text( self, tag_name ):
        result = self.get_markup_words( tag_name )
        return string.join( result )

    def  get_markup_items( self, tag_name ):
        try:
            m = self.get_markup( tag_name )
            return m.fields[0].items
        except:
            return None


