
















































var FILE_BUGNUMBERS   = "";
var window;
var PASSED;
var FAILED;

function AddTestCase( s, e, a, n, b, i ) {
    TESTCASES[TESTCASES.length] = new TestCase( s, e, a, b, n, i );
    return TESTCASES[TESTCASES.length];
}
function TestCase( s, e, a, n, b, i ) {
    this.id = ID++;
    this.description = s;
    this.expected = e;
    this.actual = a;
    if ( n )
        this.negative = n;
    if ( b )
        this.bugnumber = b;
    if ( i )
        this.ignore = i;
    this.passed = GetResult( e, a );
}

function StartTest( t ) {
    TESTCASES =  new Array();
    FILE_FAILED_CASES = 0;
    FILE_PASSED_CASES = 0;
    FILE_PASSED       = true;
    COMPLETED         = false;
    ID                = 0;

    FILE_TITLE = t;

    WriteLine("\n" + FILE_TITLE  +"\n");

    if ( window ) {
        document.open();
        PASSED = "<font color=\"#00cc00\">passed </font>";
        FAILED = "<font color=\"#ff0000\">FAILED </font>";

    } else {
        PASSED = "passed ";
        FAILED = "FAILED ";
    }
}
function StopTest() {
    
    writeReadableResults();
    
    writeTestFileSummary();

    if ( window ) {
        document.close();
    }
}
function AddComment(s) {
    WriteLine(s);
}

function writeReadableResults() {
    for ( var i = 0; i < TESTCASES.length; i++ ) {
        var tc = TESTCASES[i];
        if (tc.passed && this.DONT_PRINT_PASSED_TESTS)
            continue;
        WriteLine(
            (tc.passed ? PASSED : FAILED) +
            tc.description + " = " +
            tc.actual      + " " +
            (tc.passed ? "" : "expected " + tc.expected)
        );
    }
}

function writeParseableResults() {
    WriteLine( "START TEST CASE RESULTS" );
    for ( var i = 0; i < TESTCASES.length; i++ ) {
        var tc = TESTCASES[i];

        WriteLine( tc.id +","+
                   tc.description +","+
                   tc.expected +","+
                   tc.actual +","+
                   tc.bugnumber +","+
                   tc.negative  +","+
                   tc.ignore    +","+
                   tc.exception +","+
                   tc.passed );
    }
}
function writeTestFileSummary() {
    WriteLine ("\nTEST FILE SUMMARY" );

    WriteLine( "Title:        " + FILE_TITLE );
    WriteLine( "Passed:       " + FILE_PASSED );
    WriteLine( "Testcases:    " + TESTCASES.length );
    WriteLine( "Passed Cases: " + FILE_PASSED_CASES );
    WriteLine( "Failed Cases: " + FILE_FAILED_CASES );

    
    var gc;
    if ( typeof gc == "function") {
        gc();
    }
}
function GetResult(expect, actual) {
    if ( actual != actual ) {
        if ( typeof actual == "object" ){
            actual = "NaN object";
        } else {
            actual = "NaN number";
        }
    }
    if ( expect != expect ) {
        if ( typeof expect == "object" ) {
            expect = "NaN object";
        } else {
            expect = "NaN number";
        }
    }

    var passed = ( expect == actual ) ? true : false;

    if ( typeof(expect) != typeof(actual) ) {
        passed = false;
    }

    if ( !passed ) {
        FILE_PASSED = false;
        FILE_FAILED_CASES++;
    } else {
        FILE_PASSED_CASES++;
    }


    return passed;
}

function PrintResult(e, a, s, p) {
}

function PrintHTMLFormattedResult( e, a, s, p ) {
}
function WriteLine( s ) {
    if ( window ) {
        document.write( s +"<br>");
    } else {
        print ( s );
    }
}

function GetFailedCases() {
    for ( var i = 0; i < TESTCASES.length; i++ ) {
        var tc = TESTCASES[i];

        if ( !tc.passed )
        WriteLine(
            (tc.passed ? "passed  " : "FAILED! ") +
            tc.description + " = " +
            tc.actual      + " " +
            (tc.passed ? "" : "expected " + tc.expected)
        );
    }
}





function Enumerate( o ) {
    var p;
    WriteLine( "Properties of object " + o );
    for ( p in o ) {
        WriteLine( p +": "+ (typeof o[p] == "function" ? "function" : o[p]) );
    }
}








var GLOBAL = "[object global]";

