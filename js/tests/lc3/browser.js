



















































GLOBAL = this + '';

var TT = "<tt>";
var TT_ = "</tt>";
var BR = "<br>";
var NBSP = "&nbsp;";
var CR = "\n";
var FONT = "";
var FONT_ = "</font>";
var FONT_RED = "<font color=\"red\">";
var FONT_GREEN = "<font color=\"green\">";
var B = "<b>";
var B_ = "</b>"
var H2 = "<h2>";
var H2_ = "</h2>";
var HR = "<hr>";

function writeHeaderToLog( string ) {
  string = String(string);

  if (typeof dump == 'function')
  {
    dump( string + '\n');
  }

  string = string.replace(/[<>&]/g, htmlesc);

  DocumentWrite( "<h2>" + string + "</h2>" );
}

function writeFormattedResult( expect, actual, string, passed ) {
  string = String(string);

  if (typeof dump == 'function')
  {
    dump( string + '\n');
  }

  string = string.replace(/[<>&]/g, htmlesc);

  var s = "<tt>"+ string ;
  s += "<b>" ;
  s += ( passed ) ? "<font color=#009900> &nbsp;" + PASSED
    : "<font color=#aa0000>&nbsp;" +  FAILED + expect + "</tt>";

  DocumentWrite( s + "</font></b></tt><br>" );
  return passed;
}










window.onerror = err;
var gExceptionExpected = false;

function err( msg, page, line ) {
  var testcase;
  
  optionsPush();

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
  }
  else
  {
    if (typeof SECTION == 'undefined')
    {
      SECTION = 'Unknown';
    }
    if (typeof DESCRIPTION == 'undefined')
    {
      DESCRIPTION = 'Unknown';
    }
    if (typeof EXPECTED == 'undefined')
    {
      EXPECTED = 'Unknown';
    }

    testcase = new TestCase(SECTION, DESCRIPTION, EXPECTED, "error");
    testcase.reason += "Error: " + msg + 
      " Source File: " + page + " Line: " + line + ".";
  }
  stopTest();

  gDelayTestDriverEnd = false;
  jsTestDriverEnd();

  optionsReset();
}

var gVersion = 0;

function version(v)
{
  if (v) { 
    gVersion = v; 
  } 
  return gVersion; 
}

