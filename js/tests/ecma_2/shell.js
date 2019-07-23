










































var completed = false;
var testcases = new Array();
var tc = testcases.length;

var SECTION = "";
var VERSION = "";
var BUGNUMBER="";

var TZ_DIFF = getTimeZoneDiff();





var GLOBAL = "[object global]";
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
  if ( version ) {
    
    if ( VERSION == "ECMA_1" ) {
      version ( 130 );
    }
    if ( VERSION == "JS_13" ) {
      version ( 130 );
    }
    if ( VERSION == "JS_12" ) {
      version ( 120 );
    }
    if ( VERSION  == "JS_11" ) {
      version ( 110 );
    }
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
  var s = string ;
  s += ( passed ) ? PASSED : FAILED + expect;
  print( s);
  return passed;
}

function writeHeaderToLog( string ) {
  print( string );
}







function stopTest() {
  var gc;
  if ( gc != undefined ) {
    gc();
  }
}






function getFailedCases() {
  for ( var i = 0; i < testcases.length; i++ ) {
    if ( ! testcases[i].passed ) {
      print( testcases[i].description +" = " +testcases[i].actual +" expected: "+ testcases[i].expect );
    }
  }
}
function Enumerate ( o ) {
  var properties = new Array();
  for ( p in o ) {
    properties[ properties.length ] = new Array( p, o[p] );
  }
  return properties;
}







function getTimeZoneDiff()
{
  return -((new Date(2000, 1, 1)).getTimezoneOffset())/60;
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

