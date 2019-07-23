




































var completed = false;
var testcases;

var TT = "<tt>";
var TT_ = "<tt>";
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

function htmlesc(str) { 
  if (str == '<') 
    return '&lt;'; 
  if (str == '>') 
    return '&gt;'; 
  if (str == '&') 
    return '&amp;'; 
  return str; 
}

function DocumentWrite(s)
{
  try
  {
    var msgDiv = document.createElement('div');
    msgDiv.innerHTML = s;
    document.body.appendChild(msgDiv);
    msgDiv = null;
  }
  catch(excp)
  {
    document.write(s + '<br>\n');
  }
}

function print() { 
  var s = ''; 
  var a;
  for (var i = 0; i < arguments.length; i++) 
  { 
    a = arguments[i]; 
    s += String(a) + ' '; 
  } 

  if (typeof dump == 'function')
  {
    dump( s + '\n');
  }

  s = s.replace(/[<>&]/g, htmlesc);

  DocumentWrite(s);
}

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

function ToInteger( t ) {
  t = Number( t );

  if ( isNaN( t ) ){
    return ( Number.NaN );
  }
  if ( t == 0 || t == -0 ||
       t == Number.POSITIVE_INFINITY || t == Number.NEGATIVE_INFINITY ) {
    return 0;
  }

  var sign = ( t < 0 ) ? -1 : 1;

  return ( sign * Math.floor( Math.abs( t ) ) );
}
function Enumerate ( o ) {
  var p;
  for ( p in o ) {
    print( p +": " + o[p] );
  }
}










window.onerror = err;
var gExceptionExpected = false;

function err( msg, page, line ) {
  var testcase;
  var jsOptions = new JavaScriptOptions();

  jsOptions.setOption('strict', false);
  jsOptions.setOption('werror', false);

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
  stopTest();

  gDelayTestDriverEnd = false;
  jsTestDriverEnd();

}

var gVersion = 0;

function version(v)
{
  if (v) { 
    gVersion = v; 
  } 
  return gVersion; 
}

function gc()
{
  
  for (var i = 0; i != 100000; ++i)
  {
    var tmp = new Object();
  }
}

function jsdgc()
{
  try
  {
    
    netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');
    var jsdIDebuggerService = Components.interfaces.jsdIDebuggerService;
    var service = Components.classes['@mozilla.org/js/jsd/debugger-service;1'].
      getService(jsdIDebuggerService);
    service.GC();
  }
  catch(ex)
  {
    print('gc: ' + ex);
  }
}
