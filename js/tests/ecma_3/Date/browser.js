



















































var GLOBAL = this + '';

function htmlesc(str) { 
  if (str == '<') 
    return '&lt;'; 
  if (str == '>') 
    return '&gt;'; 
  if (str == '&') 
    return '&amp;'; 
  return str; 
}

function print() { 
  var s = ''; 
  var a; for (var i = 0; i < arguments.length; i++) 
  { 
    a = arguments[i]; 
    s += String(a) + ' '; 
  } 
  DocumentWrite(s);
}

function writeHeaderToLog( string ) {
  string = String(string);
  string = string.replace(/[<>&]/g, htmlesc);
  document.write( "<h2>" + string + "</h2>" );
}

function writeFormattedResult( expect, actual, string, passed ) {
  string = String(string);
  string = string.replace(/[<>&]/g, htmlesc);
  var s = "<tt>"+ string ;
  s += "<b>" ;
  s += ( passed ) ? "<font color=#009900> &nbsp;" + PASSED
    : "<font color=#aa0000>&nbsp;" +  FAILED + expect + "</tt>";
  document.write( s + "</font></b></tt><br>" );
  return passed;
}










window.onerror = err;
var gExceptionExpected = false;

function err( msg, page, line ) {
  var testcase;

  if (typeof(EXPECTED) == "undefined" || EXPECTED != "error") {
    


    print( "Test failed with the message: " + msg );
    testcase = new TestCase(SECTION, "unknown", "unknown", "unknown");
    testcase.passed = false;
    testcase.reason = "Error: " + msg + 
      " Source File: " + page + " Line: " + line + ".";
    if (document.location.href.indexOf('-n.js') != -1)
    {
      
      testcase.passed = true;
    }
    return;
  }

  testcase = new TestCase(SECTION, DESCRIPTION, EXPECTED, "error");
  testcase.reason += "Error: " + msg + 
    " Source File: " + page + " Line: " + line + ".";
  stopTest();
}

var gVersion = 0;

function version(v)
{
  if (v) { 
    gVersion = v; 
  } 
  return gVersion; 
}
