



package com.netscape.javascript.qa.drivers;

import java.io.File;

import org.mozilla.javascript.*;




















































public class RhinoDrv extends TestDriver {
    public final String SUFFIX = ".js";
    
    



    
    public RhinoDrv( String[] args) {
        super( args );
        setSuffix(".js");
       
    }
    public static void main ( String[] args ) {
        RhinoDrv d = new RhinoDrv( args );
        d.start();
    }
    public boolean processOptions() {
        int length = ARGS.length;
        if (ARGS[0].startsWith("-")) {
            

            for (int i=0; i < ARGS.length; i++ ) {
                if ( ARGS[i].equals("-t")) {
                    String __tinderbox = ARGS[++i];
                    
                    if ( __tinderbox.equals("true") || __tinderbox.equals("1")){
                        TestDriver.TINDERBOX = true;
                    } else {
                        TestDriver.TINDERBOX = false;
                    }
                }
                
                if ( ARGS[i].equals("-d") ) {
                    p( "-d "  );

                    this.TEST_DIRECTORY = ARGS[i].endsWith(File.separator)
                    ? new File( ARGS[++i] )
                    : new File( ARGS[++i] + File.separator );
                    p( "-d " +this.TEST_DIRECTORY );

                    if ( ! ( this.TEST_DIRECTORY ).isDirectory() ) {
                        System.err.println( "error:  " +
                        this.TEST_DIRECTORY.getAbsolutePath() +" is not a TEST_DIRECTORY." );
                        return false;
                    } else {
                        continue;
                    }
                }
                if ( ARGS[i].equals("-s") ) {
                    p( "-s ");
                    FILES = new String[20] ;
                    for ( int j = ++i, k=0; j < ARGS.length; j++ ) {
                        if ( ARGS[j].startsWith("-") ){
                            break;
                        }
                        FILES[k++] = ARGS[j];
                    }
                }
                if ( ARGS[i].equals("-h") ) {
                    p( "-h"  );
                    this.HELPER_FUNCTIONS = new File( ARGS[++i] );
                    if ( ! (this.HELPER_FUNCTIONS ).isFile() ) {
                        System.err.println( "error:  "+ this.HELPER_FUNCTIONS.getAbsolutePath()+
                        " file not found." );
                        return false;
                    }
                }
                if ( ARGS[i].equals("-o") ) {
                    p( "-o" );
                    OUTPUT_DIRECTORY = new File(ARGS[++i]+File.separator);
                    if ( !OUTPUT_DIRECTORY.exists() || !OUTPUT_DIRECTORY.isDirectory() ) {
                        System.err.println( "error:  "+OUTPUT_DIRECTORY.getAbsolutePath()+
                        " is not a directory.");
                        return false;
                    }
                }

                if ( ARGS[i].equals("-p") ) {
                    OPT_LEVEL = Integer.parseInt( ARGS[++i] );
                }

                if ( ARGS[i].equals("-db" )) {
                    DEBUG_LEVEL = Integer.parseInt( ARGS[++i] );
                    OPT_LEVEL = 0;
                }

                if ( ARGS[i].equals("-e")) {
                    EXECUTABLE = ARGS[++i];
                }
            }

            return true;

        } else {

            switch ( ARGS.length ) {
            case 0:
                System.err.println( "error:  specify location of JavaScript "+
                "tests" );
                return false;
            case 1:
                System.err.println( "error:  specify location of JavaScript "+
                "HELPER_FUNCTIONS file" );
                return false;
            case 2:
                this.TEST_DIRECTORY = ARGS[0].endsWith(File.separator)
                ? new File( ARGS[0] )
                : new File( ARGS[0] + File.separator );
                this.HELPER_FUNCTIONS = new File( ARGS[1] );
                if ( ! ( this.TEST_DIRECTORY ).isDirectory() ) {
                    System.err.println( "error:  " +
                    this.TEST_DIRECTORY.getAbsolutePath() +" is not a directory." );
                    return false;
                }
                if ( ! (this.HELPER_FUNCTIONS ).isFile() ) {
                    System.err.println( "error:  "+ this.HELPER_FUNCTIONS.getAbsolutePath()+
                    " file not found." );
                    return false;
                }
                return true;
            default:
                System.err.println( "could not understand arguments." );
                return false;
            }
        }
    }
    
    


     
    public synchronized void executeSuite( TestSuite suite ) {
        RhinoEnv context;
        TestFile file;
        
        for ( int i = 0; i < suite.size(); i++ ) {
            synchronized ( suite ) {
                file = (TestFile) suite.elementAt( i );

                context = new RhinoEnv( file, suite, this );
                
                context.runTest();
                
                writeFileResult( file, suite, OUTPUT_DIRECTORY );
                writeCaseResults(file, suite, OUTPUT_DIRECTORY );
                context.close();
                context = null;

                if ( ! file.passed ) {
                    suite.passed = false;
                }
            }
        }
        writeSuiteResult( suite, OUTPUT_DIRECTORY );
        writeSuiteSummary( suite, OUTPUT_DIRECTORY );
    }    
}    