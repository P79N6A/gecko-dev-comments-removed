var completed = false;
var testcases;

SECTION = "";
VERSION = "";

BUGNUMBER ="";
var EXCLUDE = "";

TZ_DIFF = -8;

var TT = "";
var TT_ = "";
var BR = "";
var NBSP = " ";
var CR = "\n";
var FONT = "";
var FONT_ = "";
var FONT_RED = "";
var FONT_GREEN = "";
var B = "";
var B_ = ""
var H2 = "";
var H2_ = "";
var HR = "";
var DEBUG = false;

version(130);

var PASSED = " PASSED!"
var FAILED = " FAILED! expected: ";

function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+
                            testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}

function TestCase( n, d, e, a ) {
    this.name        = n;
    this.description = d;
    this.expect      = e;
    this.actual      = a;
    this.passed      = true;
    this.reason      = "";

    this.bugnumber   = BUGNUMBER;

    this.passed = getTestCaseResult( this.expect, this.actual );
    if ( DEBUG ) {
        print( "added " + this.description );
    }
}
function startTest() {
    
    if ( VERSION == "ECMA_1" ) {
        version ( "130" );
    }
    if ( VERSION == "JS_1.3" ) {
        version ( "130" );
    }
    if ( VERSION == "JS_1.2" ) {
        version ( "120" );
    }
    if ( VERSION  == "JS_1.1" ) {
        version ( "110" );
    }
    
    
}
function getTestCaseResult( expect, actual ) {
    
    
        if ( actual != actual ) {
            if ( typeof actual == "object" ) {
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

    
    
        if (    !passed
                && typeof(actual) == "number"
                && typeof(expect) == "number"
            ) {
                if ( Math.abs(actual-expect) < 0.0000001 ) {
                    passed = true;
                }
        }

    
        if ( typeof(expect) != typeof(actual) ) {
            passed = false;
        }

        return passed;
}
function writeTestCaseResult( expect, actual, string ) {
        var passed = getTestCaseResult( expect, actual );
        writeFormattedResult( expect, actual, string, passed );
        return passed;
}
function writeFormattedResult( expect, actual, string, passed ) {
        var s = TT + string ;

        for ( k = 0;
              k <  (60 - string.length >= 0 ? 60 - string.length : 5) ;
              k++ ) {
        }

        s += B ;
        s += ( passed ) ? FONT_GREEN + NBSP + PASSED : FONT_RED + NBSP + FAILED + expect + TT_ ;

        print( s + FONT_ + B_ + TT_ );

        return passed;
}

function writeHeaderToLog( string ) {
    print( H2 + string + H2_ );
}
function stopTest()
{
    var sizeTag  = "<#TEST CASES SIZE>";
    var doneTag  = "<#TEST CASES DONE>";
    var beginTag = "<#TEST CASE ";
    var endTag   = ">";

    print(sizeTag);
    print(testcases.length);
    for (tc = 0; tc < testcases.length; tc++)
    {
        print(beginTag + 'PASSED'      + endTag);
        print(testcases[tc].passed);
        print(beginTag + 'NAME'        + endTag);
        print(testcases[tc].name);
        print(beginTag + 'EXPECTED'    + endTag);
        print(testcases[tc].expect);
        print(beginTag + 'ACTUAL'      + endTag);
        print(testcases[tc].actual);
        print(beginTag + 'DESCRIPTION' + endTag);
        print(testcases[tc].description);
        print(beginTag + 'REASON'      + endTag);
        print(( testcases[tc].passed ) ? "" : "wrong value ");
        print(beginTag + 'BUGNUMBER'   + endTag);
        print( BUGNUMBER );
    }
    print(doneTag);

    print( HR );
    gc();
}
function getFailedCases() {
  for ( var i = 0; i < testcases.length; i++ ) {
     if ( ! testcases[i].passed ) {
        print( testcases[i].description +" = " +testcases[i].actual +" expected: "+ testcases[i].expect );
     }
  }
}
function err( msg, page, line ) {
    testcases[tc].actual = "error";
    testcases[tc].reason = msg;
    writeTestCaseResult( testcases[tc].expect,
                         testcases[tc].actual,
                         testcases[tc].description +" = "+ testcases[tc].actual +
                         ": " + testcases[tc].reason );
    stopTest();
    return true;
}

function Enumerate ( o ) {
    var p;
    for ( p in o ) {
        print( p +": " + o[p] );
    }
}
