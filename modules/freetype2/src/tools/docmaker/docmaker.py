














from sources   import *
from content   import *
from utils     import *
from formatter import *
from tohtml    import *

import utils

import sys, os, time, string, glob, getopt


def  usage():
    print "\nDocMaker Usage information\n"
    print "  docmaker [options] file1 [file2 ...]\n"
    print "using the following options:\n"
    print "  -h : print this page"
    print "  -t : set project title, as in '-t \"My Project\"'"
    print "  -o : set output directory, as in '-o mydir'"
    print "  -p : set documentation prefix, as in '-p ft2'"
    print ""
    print "  --title  : same as -t, as in '--title=\"My Project\"'"
    print "  --output : same as -o, as in '--output=mydir'"
    print "  --prefix : same as -p, as in '--prefix=ft2'"


def  main( argv ):
    """main program loop"""

    global output_dir

    try:
        opts, args = getopt.getopt( sys.argv[1:], \
                                    "ht:o:p:",    \
                                    ["help", "title=", "output=", "prefix="] )
    except getopt.GetoptError:
        usage()
        sys.exit( 2 )

    if args == []:
        usage()
        sys.exit( 1 )

    
    
    project_title  = "Project"
    project_prefix = None
    output_dir     = None

    for opt in opts:
        if opt[0] in ( "-h", "--help" ):
            usage()
            sys.exit( 0 )

        if opt[0] in ( "-t", "--title" ):
            project_title = opt[1]

        if opt[0] in ( "-o", "--output" ):
            utils.output_dir = opt[1]

        if opt[0] in ( "-p", "--prefix" ):
            project_prefix = opt[1]

    check_output()

    
    source_processor  = SourceProcessor()
    content_processor = ContentProcessor()

    
    file_list = make_file_list( args )
    for filename in file_list:
        source_processor.parse_file( filename )
        content_processor.parse_sources( source_processor )

    
    content_processor.finish()

    formatter = HtmlFormatter( content_processor, project_title, project_prefix )

    formatter.toc_dump()
    formatter.index_dump()
    formatter.section_dump_all()




if __name__ == '__main__':
    main( sys.argv )



