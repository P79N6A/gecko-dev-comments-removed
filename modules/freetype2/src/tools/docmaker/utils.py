














import string, sys, os, glob, itertools




output_dir = None













def  index_key( s ):
    return string.join( itertools.chain( *zip( s.lower(), s ) ) )




def  sort_order_list( input_list, order_list ):
    new_list = order_list[:]
    for id in input_list:
        if not id in order_list:
            new_list.append( id )
    return new_list






def  open_output( filename ):
    global output_dir

    if output_dir and output_dir != "":
        filename = output_dir + os.sep + filename

    old_stdout = sys.stdout
    new_file   = open( filename, "w" )
    sys.stdout = new_file

    return ( new_file, old_stdout )




def  close_output( output ):
    output[0].close()
    sys.stdout = output[1]




def  check_output():
    global output_dir
    if output_dir:
        if output_dir != "":
            if not os.path.isdir( output_dir ):
                sys.stderr.write( "argument"
                                  + " '" + output_dir + "' "
                                  + "is not a valid directory" )
                sys.exit( 2 )
        else:
            output_dir = None


def  file_exists( pathname ):
    """Check that a given file exists."""
    result = 1
    try:
        file = open( pathname, "r" )
        file.close()
    except:
        result = None
        sys.stderr.write( pathname + " couldn't be accessed\n" )

    return result


def  make_file_list( args = None ):
    """Build a list of input files from command-line arguments."""
    file_list = []
    

    if not args:
        args = sys.argv[1:]

    for pathname in args:
        if string.find( pathname, '*' ) >= 0:
            newpath = glob.glob( pathname )
            newpath.sort()  
                            
        else:
            newpath = [pathname]

        file_list.extend( newpath )

    if len( file_list ) == 0:
        file_list = None
    else:
        
        file_list = filter( file_exists, file_list )

    return file_list


