




package com.netscape.javascript.qa.drivers;

import java.util.Vector;
import java.util.Date;
import java.io.*;
import java.applet.Applet;












































public class TestDriver extends Applet implements Runnable {
    Thread THREAD = null;

    String[] SYSTEM;

    File TEST_DIRECTORY;
    File HELPER_FUNCTIONS;
    File OUTPUT_DIRECTORY;
    
    String HTTP_PATH;
    String SUFFIX;

    int OPT_LEVEL;
    int DEBUG_LEVEL;

    String[] ARGS;

    String EXECUTABLE = null;

    Vector SUITES;
    String[] FILES;
    
    public static boolean TINDERBOX = false;

    public static final boolean DEBUG = true;
    public static final boolean TCMS = false;

    Runtime RUNTIME;

    long FREE_MEMORY;
    long TOTAL_MEMORY;

    public TestDriver( String[] args ) {
        this.ARGS = args;        
        if ( THREAD == null ) {
            THREAD = new Thread(this);
            THREAD.start();
        }
    }
    public void run() {
    }
    
    public void start() {
        this.FILES = null;
        this.OUTPUT_DIRECTORY = null;
        this.OPT_LEVEL = 0;
        this.DEBUG_LEVEL=0;
        this.RUNTIME = Runtime.getRuntime();
        this.FREE_MEMORY = RUNTIME.freeMemory();
        this.TOTAL_MEMORY = RUNTIME.totalMemory();
        
        if ( processOptions() ) {

            openLogFiles( OUTPUT_DIRECTORY == null ? TEST_DIRECTORY :
                OUTPUT_DIRECTORY);
                
            writeDateToLogs("<hr>", OUTPUT_DIRECTORY );
            writeLogHeaders( OUTPUT_DIRECTORY );
                
            if (FILES == null) {
                FILES = TEST_DIRECTORY.list();
            }

            if ( TestDriver.TINDERBOX ) {
                writeTinderboxHeader(EXECUTABLE);
            }

            SUITES = getSuites( FILES );
            
            for ( int i = 0; i < SUITES.size(); i++ ) {
                getCases((TestSuite) SUITES.elementAt(i));
            }
            for ( int i = 0; i < SUITES.size(); i++ ) {
                if (TestDriver.TINDERBOX) {
                    writeTinderboxSuiteName((TestSuite) SUITES.elementAt(i));
                }
                executeSuite((TestSuite) SUITES.elementAt(i));
                if (TestDriver.TINDERBOX) {
                    writeTinderboxSuiteResult((TestSuite) SUITES.elementAt(i));
                }
                RUNTIME.gc();
            }
        }
        stop();
    }
    


    
    public void stop() {
        closeLogs();
        RUNTIME.gc();
        long NEW_FREE_MEMORY = RUNTIME.freeMemory();
        long NEW_TOTAL_MEMORY = RUNTIME.totalMemory();
        
        String string = "<tt>"+
                        "Free Memory " + NEW_FREE_MEMORY + "\n<br>"+
                        "Total Memory " + NEW_TOTAL_MEMORY +"\n<br>"+
                        "Free Leaked  " + (FREE_MEMORY - NEW_FREE_MEMORY) +"\n<br>"+
                        "Total Leaked " + (TOTAL_MEMORY- NEW_TOTAL_MEMORY) +"\n<br>"+
                        "</tt>";
                        
        p( string );                        
        p("done!");
        
        if ( THREAD != null ) {
            THREAD.stop();
            THREAD = null;
        }            
    }
    
    public void closeLogs() {
        writeDateToLogs( "", OUTPUT_DIRECTORY );        
        getLog( OUTPUT_DIRECTORY, SUMMARY_LOG_NAME ).closeLog();
        getLog( OUTPUT_DIRECTORY, SUITE_LOG_NAME ).closeLog();
        getLog( OUTPUT_DIRECTORY, FILE_LOG_NAME).closeLog();
        getLog( OUTPUT_DIRECTORY, CASE_LOG_NAME).closeLog();

        if ( DEBUG ) {
            getLog( OUTPUT_DIRECTORY, DEBUG_LOG_NAME ).closeLog();
        }
    }        
    
    


    public static String[] getSystemInformation() {
        String [] SYSTEM = {  System.getProperty( "os.name" ),
                        System.getProperty( "os.arch" ),
                        System.getProperty( "os.version" ) };
        return SYSTEM;
    }

    





    public static TestLog getLog(File output, String filename ) {
        enablePrivileges();
        TestLog log = null;
        String[] SYSTEM = getSystemInformation();        



        String platform = SYSTEM[0];

        try {
            File logdir = new File( 
                output.getAbsolutePath() +
                (output.getAbsolutePath().endsWith(File.separator) ? "" : File.separator) +
                platform + 
                SYSTEM[1] +"-"+ SYSTEM[2],
                getCurrentDate("_") );
            logdir.mkdirs();
            log = new TestLog( logdir.getAbsolutePath() + File.separator +
                                    filename,   TERMINATOR );
        } catch ( Exception e ) {
            p( "TestDriver.getLog threw " + e.toString() );
            p( "platform " + platform );
            p( "output" + output.toString() );
            p( "filename " + filename.toString() );
            p( "File.separator " + File.separator.toString() );
            p( "SYSTEM[0] " + SYSTEM[0] );
            p( "SYSTEM[1] " + SYSTEM[1] );
            p( "SYSTEM[2] " + SYSTEM[2] );
            p( "date " +getCurrentDate("_") );
           
            e.printStackTrace();
        }
        
        return log;

    }        
    
    




    
    public static void openLogFiles( File output ) {
        enablePrivileges();
        
        TestLog SUMMARY_LOG = getLog( output, SUMMARY_LOG_NAME );
        TestLog FILE_LOG    = getLog( output, FILE_LOG_NAME );
        TestLog SUITE_LOG   = getLog( output, SUITE_LOG_NAME );
        TestLog CASE_LOG    = getLog( output, CASE_LOG_NAME );

        if ( DEBUG ) {
            TestLog DEBUG_LOG = getLog( output, DEBUG_LOG_NAME );
        }

        return;
    }
    
    public void writeLogHeaders( File output ) {
        String header = "<table>"+
            "<tr><th>Test Driver <td>" + this.getClass().toString() +"</tr>" +
            "<tr><th>Output Directory <td>" + OUTPUT_DIRECTORY   +"</tr>" +
            ( EXECUTABLE == null 
                ?  "<tr><th>OptLevel <td>"           + OPT_LEVEL +"</tr>" +
                   "<tr><th>DebugLevel <td>"           + DEBUG_LEVEL+"</tr>"
                : "<tr><th>Executable <td>"           + EXECUTABLE +"</tr>"                        
            ) +
            "<tr><th>Java Version        <td>" + System.getProperty("java.vendor")+
                " "+ System.getProperty( "java.version" )           +"</tr>"+
            "<tr><th>Test File Directory <td>" + TEST_DIRECTORY     +"</tr>"+
            "<tr><th>Helper Functions <td>"    + HELPER_FUNCTIONS   +"</tr>"+
            "<tr><th>Free Memory      <td>"    + FREE_MEMORY        +"</tr>"+
            "<tr><th>Total JVM Memory <td>"    + TOTAL_MEMORY       +"</tr>"+
            "</table>";

        getLog( OUTPUT_DIRECTORY, SUITE_LOG_NAME ).writeLine( header );
        getLog( OUTPUT_DIRECTORY, CASE_LOG_NAME  ).writeLine( header );
        getLog( OUTPUT_DIRECTORY, FILE_LOG_NAME ).writeLine( header );
        getLog( OUTPUT_DIRECTORY, SUMMARY_LOG_NAME ).writeLine( header );        
        
        getLog( OUTPUT_DIRECTORY, SUMMARY_LOG_NAME ).writeLine( 
                                "<table width=100%>" +
                               "<tr bgColor=#ffffcc >" +
                               "<th rowspan=2 width=15%> Suite" +
                               "<th rowspan=2 width=20%> Files" +
                               "<th rowspan=2 width=10%> Cases" +
                               "<th colspan=2 width=20% > Passed" +
                               "<th colspan=2 width=20%> Failed" +
                               "<th rowspan=2 width=15%> Result" +
                               "</tr>" +
                               "<tr>" +
                               "<th width=10%> #" +
                               "<th width=10%> %" +
                               "<th width=10%> #" +
                               "<th width=10%> %" +
                               "</tr></table>" );
        return;                               
    }
    
    



    
    public Vector getSuites( String[] files ) {
        Vector suites   = new Vector();

        for ( int i = 0; i < files.length; i++ ) {
            String filename = stripDoubleSlashes( TEST_DIRECTORY +
                 File.separator + files[i] );
            File fileobject = new File( filename );
            
            if ( fileobject.isDirectory() ) {
                TestSuite s = new TestSuite( fileobject.getName(),
                    filename );
                suites.addElement( s );
            }
        }

        return suites;
    }
    




    public static String stripDoubleSlashes( String string ) {
        StringBuffer buffer = new StringBuffer();
        for ( int letter = 0; letter < string.length(); letter++ ) {
            char current_char = string.charAt(letter);
            
            if ( current_char != File.separatorChar ) {
                buffer.append(current_char);
            } else {
                while ( string.charAt(++letter) == 
                    File.separatorChar )
                {                    
                    ;
                }                    
                
                buffer.append( File.separatorChar );
                --letter;
            }
            
        }            
        










            


        return buffer.toString();
    }

    




    public void getCases( TestSuite suite ) {
        enablePrivileges();
        
        File dir = new File ( suite.filePath );
        String[] files      = dir.list();

        for ( int i = 0; i < files.length; i++ ) {
            TestFile item = new TestFile( files[i],
                suite.filePath + File.separator+ files[i] );

            if ( item.isFile()
                 && (item.getName().toLowerCase()).endsWith(getSuffix())
                 && (!item.isDirectory()) ) {
                suite.addElement(item);
                p( item +"" );
            }                
        }
    }
    








    public synchronized void executeSuite( TestSuite suite ) {
    }

    






    public static void enablePrivileges() {
    }

     






     
    





    public void writeTinderboxHeader( String executable ) {
        String dottedLine = 
        "--------------------------------------------------------------";
        
        String product = ( executable == null ) ? "ns/js/rhino" : "ns/js/ref";
        String osInfo = System.getProperty( "os.name" ) +" " + 
                        System.getProperty( "os.arch" ) +" " +
                        System.getProperty( "os.version" );
        String javaInfo=System.getProperty( "java.vendor" ) +" "+
                        System.getProperty( "java.version" );
        
        
        
        System.out.println( dottedLine );
        System.out.println( "Product            " + product );

        System.out.println( "Operating System   " + osInfo  );
        System.out.println( "Java Version       " + javaInfo);
        System.out.println( "Free Memory        " + FREE_MEMORY );
        System.out.println( "Total JVM Memory   " + TOTAL_MEMORY );
                                                       
        System.out.println( "Test Driver        " + this.getClass().getName());
        System.out.println( "Output Directory   " + OUTPUT_DIRECTORY );
        System.out.println( "Path to Executable " + EXECUTABLE );
        
        if ( product.equals( "ns/js/rhino" ) ){
            System.out.println( "Optimization Level " + OPT_LEVEL );
            System.out.println( "Debug level        " + DEBUG_LEVEL );
        }

        
        System.out.println( "Test Directory     " + TEST_DIRECTORY );
        System.out.println( dottedLine );        
    }
    

    public void writeTinderboxSuiteName( TestSuite suite ) {
        System.out.println( "Suite              " + suite.name );
    }
        
    public void writeTinderboxSuiteResult( TestSuite suite ) {
        String dottedLine = 
        "--------------------------------------------------------------";

        System.out.println( "Result             " + ( suite.passed 
            ? "PASSED" : "FAILED"));

        if ( ! suite.passed ) {
            System.out.println( "Failed Files       " );
            for ( int i = 0; i < suite.size(); i++ ) {
                TestFile file = (TestFile) suite.elementAt(i);
                if ( ! file.passed ) {
                    System.out.println(  file.name  + 
                        (file.bugnumber.equals("") ? "" :
                            " http://scopus.mcom.com/bugsplat/show_bug.cgi?id="+
                            file.bugnumber +" "));
                }
            }
        }        
        
        System.out.println( dottedLine );    
    }
    
    


    public static void writeSuiteResult( TestSuite suite, File output ) {
        if ( ! (suite.size() > 0) ) {
            return;
        }

        String buffer = "";
        buffer += ( "<tt>" +
                        ( suite.passed
                        ? "<font color=#00dd00>passed</font>"
                        : "<font color=#dd0000>FAILED</font>"
                        ) +
                        "&nbsp;" +
                        "<a href='./../../" + suite.name +"'>"+
                        suite.name +"</a></b>" );

        if ( ! suite.passed ) {
            for ( int i = 0; i < suite.size(); i++ ) {
                TestFile file = (TestFile) suite.elementAt(i);
                if ( ! file.passed ) {
                    buffer += (  "&nbsp;" +
                                    "<a href='case.html#"+ suite.name +"-"+ file.name + "'>" +
                                    file.name +  "</a>" );
                                    
                    if ( ! file.bugnumber.equals("") ) {
                        buffer += ( "&nbsp;"+
                            "(<a href='http://scopus.mcom.com/bugsplat/show_bug.cgi?id="+
                            file.bugnumber+"'>"+file.bugnumber+"</a>)" );
                    }                        
                }
            }
        }

        buffer += ( "</tt><br >" );

        getLog( output, SUITE_LOG_NAME).writeLine( buffer.toString() );
    }

    








    public static void writeSuiteSummary( TestSuite suite, File output ) {
        if ( ! (suite.size() > 0) ) {
            return;
        }

        String buffer = "";
        p( "total :"+ suite.totalCases );
        p("passed :"+  suite.casesPassed );
        p("failed :"+ suite.casesFailed );


        double percentPassed = Math.round((double) suite.casesPassed / 
                                (double) suite.totalCases * 10000)/100 ;
        double percentFailed = Math.round((double) suite.casesFailed / 
                                (double) suite.totalCases * 10000)/100 ;

        buffer += (  "<table  width=100%>" );
        buffer += (  "<th bgcolor=#ffffcc width=15%>" +
                         suite.name );

        buffer += (  "<th width=15% bgcolor=#ffffcc >" + suite.size() +
                        "<th width=10% bgcolor=#ffffcc >" + suite.totalCases +
                        "<th width=10% bgcolor=#ffffcc >" + suite.casesPassed +
                        "<th width=10% bgcolor=#ffffcc >" + percentPassed +
                        "<th width=10% bgcolor=#ffffcc >" + suite.casesFailed +
                        "<th width=10% bgcolor=#ffffcc >" + percentFailed +
                        "<th width=15% bgcolor=#ffffcc >" +
                       (    ( suite.passed == true )
                            ? "<font color=#006600 >PASSED</font >"
                            : "<font color=#990000 >FAILED</font >"
                        ) + "</tr></table>" );

        for ( int i = 0; i < suite.size(); i++ ) {
            TestFile file= ((TestFile) suite.elementAt(i));

            double pPassed = 0;
            double pFailed = 0;

            if ( file.totalCases > 0 ) {
                pPassed = (Math.round((double) file.casesPassed / 
                            (double) file.totalCases * 10000))/100 ;
                pFailed = (Math.round((double) file.casesFailed / 
                            (double) file.totalCases * 10000))/100 ;
            }

            buffer += (  "<table  width=100%>" +
                            "<tr>" +
                            "<td width=15%>" +
                            "<td width=15% ><a href='case.html#"+ suite.name +
                            "-"+ file.name + "'>" + file.name + "</a>" +
                            "<td width=10%>" + file.totalCases +
                            "<td width=10% bgColor=#ccffcc >" + file.casesPassed +
                            "<td width=10% bgColor=#ccffcc >" + pPassed +
                            "<td width=10% bgColor=#ffcccc >" + file.casesFailed +
                            "<td width=10% bgColor=#ffcccc >" + pFailed +
                            "<td width=15%>" +
                                (   ( file.passed == true )
                                    ? "<font color=#006600 >PASSED</font >"
                                    : "<font color=#990000 >FAILED</font >"
                                ) +
                            ( ( file.reason.equals("") )
                                 ? ""
                                 : ":  "+ file.reason
                            ) +
                            ( ( file.exception.equals("") )
                              ? ""
                              : ":<br>"+ file.exception
                            ) +
                            "<tr ></table>" );
        }

       getLog( output, SUMMARY_LOG_NAME ).writeLine( buffer.toString() );
    }
    
    




    public static void writeFileResult( TestFile file, TestSuite suite, File output ) {
        String suitename = ( suite == null ) ? "" : suite.name;
        
        if ( !file.passed ) {
            String buffer = "";

            buffer += ( "<tt ><font color=" +
                        ( (file.passed )
                        ? "#00dd00>PASSED</font>&nbsp;"
                        : "#dd0000>FAILED</font>&nbsp;" ) +
                        "<a href='case.html#" + suitename +"-"+ file.name + "'>" +
                        suitename+" "+file.name + "</a > " +
                        file.exception + "\n" );
                        
            if ( ! file.bugnumber.equals("") ) {
                buffer += ( "&nbsp;"+
                    "(<a href='http://scopus.mcom.com/bugsplat/show_bug.cgi?id="+
                    file.bugnumber+"'>"+file.bugnumber+"</a>)" );
            }                    
            getLog( output, FILE_LOG_NAME ).writeLine( buffer.toString() );

        }                        
    }

    


    public static String readFile( String filePath ) {
        File jsFile = new File( filePath );

        int length = new Long( jsFile.length()).intValue();
        byte[]  b =  new byte[length];
        StringBuffer contents = new StringBuffer();

        try {
            FileInputStream fis = new FileInputStream( jsFile );
            int read = fis.read( b );
            contents.append( new String( b ) );
        } catch ( Exception e ) {
            p( e.toString() );
        }
        return ( contents.toString() );
    }
    


























    public static void writeCaseResults( TestFile file, TestSuite suite, 
        File output ) 
    {
        suite.totalCases += file.totalCases;

        String[] SYSTEM = getSystemInformation();
        
        String buffer = "";            
        
        for ( int i = 0; i < file.caseVector.size(); i++ ) {
            TestCase tcase = (TestCase) (file.caseVector.elementAt(i));            
    
            
            if ( !tcase.passed.equals("true")) {
                if ( !file.passed ) {
                buffer += ("<a name=" + suite.name +"-"+ file.name +">");        
                buffer += ("<tt>");
                buffer += ( "(" + SYSTEM[0] + "), " );
                }
            }                

            if ( tcase.passed.equals("true")) {
                if (!file.passed)
                   buffer += ( "-pass-, " );
                suite.casesPassed += 1;
                file.casesPassed += 1;
            } else {
                if (!file.passed)
                    buffer += ( "*FAIL*, ");
                suite.casesFailed += 1;
                file.casesFailed += 1;
            }

            if ( TCMS ) {
                buffer += (  tcase.name + "-" + tcase.description + "/Beaker, " +
                    tcase.name + "-" + tcase.description + ", " +
                    getCurrentDate("/") + ", " +
                    file.startTime + ", " +
                    file.endTime + ", " +
                    tcase.expect +", " +
                    tcase.actual +", " +
                    tcase.reason + "\n" );
            } else {
                buffer += (  tcase.name +","+
                    tcase.description +","+
                    tcase.expect +","+
                    tcase.actual +","+
                    tcase.reason +"\n" );
            }
            
            if (!tcase.passed.equals("true")) {
                buffer += ("</tt><br>\n");
                getLog( output, CASE_LOG_NAME ).writeLine( buffer.toString() );
            } else {
                buffer = "";
            }
        }
    }
  
    













    
    public static void writeCaseResults( TestFile file, String classname, 
        File output ) 
    {
        String[] system = getSystemInformation();        
        String buffer = "";

        for ( int i = 0; i < file.caseVector.size(); i++ ) {
            TestCase tcase = (TestCase) (file.caseVector.elementAt(i));            
            if ( tcase.passed.equals("true")) {

            } else {
                buffer += ("<tt>");
                buffer += ( "(" + system[0] + "), " );
                
                buffer += ( "*FAIL*, ");
                buffer += (  tcase.name +","+
                    tcase.description +","+
                    tcase.expect +","+
                    tcase.actual +","+
                    tcase.reason +"\n" );
                buffer += ("</tt><br>\n");
                getLog( output, CASE_LOG_NAME ).writeLine( buffer.toString() );
            }            
        }            
    }        
    





    public static void writeDateToLogs( String separator, File output ) {
        Date today = new Date();
        String[] SYSTEM = getSystemInformation();
        


        
        String string = "<tt>"+




                        "</tt>"+
                        separator + SYSTEM[0] +" "+ SYSTEM[1] +" "+ SYSTEM[2] +" "+
                        today.toString();

        getLog( output, SUMMARY_LOG_NAME ).writeLine( string );
        getLog( output, FILE_LOG_NAME).writeLine( string );
        getLog( output, SUITE_LOG_NAME).writeLine( string );
        if ( !TCMS ) {
            getLog( output, CASE_LOG_NAME ).writeLine( string );
        }
    }
    
    



    public static void p( String s ) {
        if ( !TestDriver.TINDERBOX ) {
            System.out.println( s );
        }
    }
    public static void debug (String s ) {
        if ( DEBUG && !TestDriver.TINDERBOX )
            System.err.println( s );
    }
    



    public static String getCurrentDate( String separator ) {
        Date today = new Date();
        String date = (today.getDate() < 10)
                    ? "0" + String.valueOf( today.getDate() )
                    : String.valueOf( today.getDate() );
        String month = (today.getMonth() + 1 < 10 )
                     ? "0" + String.valueOf( (today.getMonth() +1) )
                     : String.valueOf( today.getMonth() + 1 );
        String year = String.valueOf( today.getYear() );

        return ( month + separator + date + separator + year );
    }
    



    public static String getCurrentTime() {
        Date now = new Date();
        String hours = ( now.getHours() < 10 )
                    ? "0" + String.valueOf( now.getHours() )
                    : String.valueOf( now.getHours() );

        String minutes = ( now.getMinutes() < 10 )
                       ? "0" + String.valueOf( now.getMinutes() )
                       : String.valueOf( now.getMinutes() );

        String seconds = ( now.getSeconds() < 10 )
                       ? "0" + String.valueOf( now.getSeconds() )
                       : String.valueOf( now.getSeconds() );

        String time = hours + ":" + minutes + ":" + seconds;

        return ( time );
    }
    public String getSuffix() {
        return SUFFIX;
    }
    public void setSuffix(String s) {
        SUFFIX = s;
    }
    



    public boolean sleep(int ms) {
        try {
            THREAD.sleep( 5000 );
        } catch ( Exception e ) {
            p( "sleep failed:  " + e );
            return false;
        }
        return true;
    }
   
    



   
    
    







    public boolean processOptions() {
        return false;
    }        
    public static void main ( String[] args ) {
        TestDriver d = new TestDriver( args );
        d.start();
    }
    
    public static final String SUMMARY_LOG_NAME = "summ.html";
    public static final String CASE_LOG_NAME =    "case.html";
    public static final String FILE_LOG_NAME =    "file.html";
    public static final String SUITE_LOG_NAME =   "suite.html";
    public static final String DEBUG_LOG_NAME =   "debug.html";
    public static final String TERMINATOR     =   "<BR>\n";
    
 }
 
