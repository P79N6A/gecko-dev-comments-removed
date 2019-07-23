











































var completed = false;
var testcases = new Array();
var tc = testcases.length;

var SECTION = "";
var VERSION = "";
var BUGNUMBER="";

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



var PASSED = " PASSED!"
var FAILED = " FAILED! expected: ";

var DEBUG = false;







function AddTestCase( description, expect, actual ) {
  new TestCase( SECTION, description, expect, actual );
}






function TestCase( n, d, e, a ) {
  this.path = (typeof gTestPath == 'undefined') ? '' : gTestPath;
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
  






  
  testcases[tc++] = this;
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
  
  

  writeHeaderToLog( SECTION + " "+ TITLE);
  if ( BUGNUMBER ) {
    print ("BUGNUMBER: " + BUGNUMBER );
  }

}

function test() {
  for ( tc=0; tc < testcases.length; tc++ ) {
    try
    {
    testcases[tc].passed = writeTestCaseResult(
      testcases[tc].expect,
      testcases[tc].actual,
      testcases[tc].description +" = "+
      testcases[tc].actual );

    testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    catch(e)
    {
      print('test(): empty testcase for tc = ' + tc + ' ' + e);
    }
  }
  stopTest();
  return ( testcases );
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






function stopTest() {
  print( HR );
  var gc;
  if ( gc != undefined ) {
    gc();
  }
  completed = true;
}





function getFailedCases() {
  for ( var i = 0; i < testcases.length; i++ ) {
    if ( ! testcases[i].passed ) {
      print ( testcases[i].description +" = " +testcases[i].actual +" expected: "+ testcases[i].expect );
    }
  }
}

function optionsInit() {

  
  
  options.initvalues  = {};

  
  
  options.stackvalues = [];

  var optionNames = options().split(',');

  for (var i = 0; i < optionNames.length; i++)
  {
    var optionName = optionNames[i];
    if (optionName)
    {
      options.initvalues[optionName] = '';
    }
  }
}

function optionsClear() {
        
  
  var optionNames = options().split(',');
  for (var i = 0; i < optionNames.length; i++)
  {
    var optionName = optionNames[i];
    if (optionName)
    {
      options(optionName);
    }
  }
}

function optionsPush()
{
  var optionsframe = {};

  options.stackvalues.push(optionsframe);

  var optionNames = options().split(',');

  for (var i = 0; i < optionNames.length; i++)
  {
    var optionName = optionNames[i];
    if (optionName)
    {
      optionsframe[optionName] = '';
    }
  }

  optionsClear();
}

function optionsPop()
{
  var optionsframe = options.stackvalues.pop();

  optionsClear();

  for (optionName in optionsframe)
  {
    options(optionName);
  }

}

function optionsReset() {

  optionsClear();

  
  for (optionName in options.initvalues)
  {
    options(optionName);
  }
}

if (typeof options == 'function')
{
  optionsInit();
  optionsClear();
}

