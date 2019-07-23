




package com.netscape.javascript.qa.drivers;

import java.util.Vector;
import java.util.Date;
import java.io.*;
import java.applet.Applet;













































public class RefDrv extends TestDriver {
    String SUFFIX  = ".js";
    boolean CODE_COVERAGE = false;
    String HELPER_STRING;
    
    




    
    public RefDrv( String[] args) {
        super( args );
        setSuffix( ".js");
    }

    








     
    
    public static void main ( String[] args ) {
        RefDrv d = new RefDrv( args );
        d.start();
    }
    




    
    public boolean processOptions() {
        int length = ARGS.length;

        if (ARGS[0].startsWith("-")) {
            

            for (int i=0; i < ARGS.length; i++ ) {
                if ( ARGS[i].equals("-d") ) {
                    p( "-d "  );

                    this.TEST_DIRECTORY = ARGS[i].endsWith(File.separator)
                    ? new File( ARGS[++i] )
                    : new File( ARGS[++i] + File.separator );
                    p( "-d " +this.TEST_DIRECTORY );

                    if ( ! ( this.TEST_DIRECTORY ).isDirectory() ) {
                        p( "error:  " +
                        this.TEST_DIRECTORY.getAbsolutePath() +
                            " is not a TEST_DIRECTORY." );
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
                    this.HELPER_STRING = new String( ARGS[++i] );
                    this.HELPER_FUNCTIONS = new File( HELPER_STRING );
                    if ( ! (this.HELPER_FUNCTIONS ).isFile() ) {
                        p( "error:  "+ 
                            this.HELPER_FUNCTIONS.getAbsolutePath()+
                            " file not found." );
                        return false;
                    }
                    p( "-h " +this.HELPER_FUNCTIONS );
                }
                if ( ARGS[i].equals("-c")) {
                    p("-c");
                    this.CODE_COVERAGE = new Boolean( ARGS[++i] ).booleanValue();
                }
                if ( ARGS[i].equals("-o") ) {
                    p( "-o" );
                    OUTPUT_DIRECTORY = new File(ARGS[++i]+File.separator);
                    if ( !OUTPUT_DIRECTORY.exists() || 
                        !OUTPUT_DIRECTORY.isDirectory() ) 
                    {
                        p( "error:  "+
                            OUTPUT_DIRECTORY.getAbsolutePath()+
                            " is not a directory.");
                        return false;
                    }
                }

                if ( ARGS[i].equals("-p") ) {
                    this.OPT_LEVEL = Integer.parseInt( ARGS[++i] );
                }

                if ( ARGS[i].equals("-db" )) {
                    this.DEBUG_LEVEL = Integer.parseInt( ARGS[++i] );
                    this.OPT_LEVEL = 0;
                }

                if ( ARGS[i].equals("-e")) {
                    this.EXECUTABLE = ARGS[++i];
                }
                if ( ARGS[i].equals("-t")) {
                    String __tinderbox = ARGS[++i];
                    
                    if ( __tinderbox.equals("true") || __tinderbox.equals("1")){
                        this.TINDERBOX = true;
                    } else {
                        this.TINDERBOX = false;
                    }
                }
            }

            return true;

        } else {

            switch ( ARGS.length ) {
            case 0:
                p( "error:  specify location of JavaScript "+
                "tests" );
                return false;
            case 1:
                p( "error:  specify location of JavaScript "+
                "HELPER_FUNCTIONS file" );
                return false;
            case 2:
                this.TEST_DIRECTORY = ARGS[0].endsWith(File.separator)
                ? new File( ARGS[0] )
                : new File( ARGS[0] + File.separator );
                this.HELPER_FUNCTIONS = new File( ARGS[1] );
                if ( ! ( this.TEST_DIRECTORY ).isDirectory() ) {
                    p( "error:  " +
                    this.TEST_DIRECTORY.getAbsolutePath() +" is not a directory." );
                    return false;
                }
                if ( ! (this.HELPER_FUNCTIONS ).isFile() ) {
                    p( "error:  "+ 
                        this.HELPER_FUNCTIONS.getAbsolutePath()+
                        " file not found." );
                    return false;
                }
                return true;
            default:
                p( "could not understand arguments." );
                return false;
            }
        }
    }
    



    
    public synchronized void executeSuite( TestSuite suite ) {
        TestEnvironment context;
        TestFile file;

        p("executing suite" );
        try {
        for ( int i = 0; i < suite.size(); i++ ) {
            synchronized ( suite ) {
                file = (TestFile) suite.elementAt( i );

                if ( getSystemInformation()[0].startsWith( "Mac" )) {
                    context = new MacRefEnv( file, suite, this );
                    ((MacRefEnv) context).runTest();
                } else {                                
                    context = new RefEnv( file, suite, this );
                    ((RefEnv) context).runTest();
                }                    
            }

                writeFileResult( file, suite, OUTPUT_DIRECTORY );
                writeCaseResults(file, suite, OUTPUT_DIRECTORY );
                context.close();
                context = null;
                    
                if ( ! file.passed ) {
                    suite.passed = false;
                }
        }
        } catch ( Exception e ) {
            e.printStackTrace() ;
        }
        writeSuiteResult( suite, OUTPUT_DIRECTORY );
        writeSuiteSummary( suite, OUTPUT_DIRECTORY );
    }
}

