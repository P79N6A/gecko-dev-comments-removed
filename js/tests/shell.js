











































if (typeof version != 'undefined')
{
  version(150);
}

var FAILED = "FAILED!: ";
var STATUS = "STATUS: ";
var VERBOSE = false;
var SECT_PREFIX = 'Section ';
var SECT_SUFFIX = ' of test - ';
var callStack = new Array();

var gTestfile;
var gTestPath;
var gTestsuite;
var gTestsubsuite;
var gDelayTestDriverEnd = false;

var gTestcases = new Array();
var gTc = gTestcases.length;
var BUGNUMBER = '';
var summary = '';
var description = '';
var expected = '';
var actual = '';
var msg = '';

var SECTION = "";
var VERSION = "";
var BUGNUMBER = "";




var GLOBAL = "[object global]";
var PASSED = " PASSED!";
var FAILED = " FAILED! expected: ";

var DEBUG = false;

var DESCRIPTION;
var EXPECTED;






function AddTestCase( description, expect, actual ) {
  new TestCase( SECTION, description, expect, actual );
}





function startTest() {
  

  if ( BUGNUMBER ) {
    print ("BUGNUMBER: " + BUGNUMBER );
  }
  if ( typeof version != 'function') {
    return;
  }

  
  if ( VERSION == "ECMA_1" ) {
    version ( "130" );
  }
  else if ( VERSION == "JS_1.8"  || gTestsuite == 'js1_8') {
    version ( "180" );
  }
  else if ( VERSION == "JS_1.7"  || gTestsuite == 'js1_7') {
    version ( "170" );
  }
  else if ( VERSION == "JS_1.6"  || gTestsuite == 'js1_6') {
    version ( "160" );
  }
  else if ( VERSION == "JS_1.5"  || gTestsuite == 'js1_5') {
    version ( "150" );
  }
  else if ( VERSION == "JS_1.4"  || gTestsuite == 'js1_4') {
    version ( "140" );
  }
  else if ( VERSION == "JS_1.3"  || gTestsuite == 'js1_3') {
    version ( "130" );
  }
  else if ( VERSION == "JS_1.2"  || gTestsuite == 'js1_2') {
    version ( "120" );
  }
  else if ( VERSION  == "JS_1.1" || gTestsuite == 'js1_1') {
    version ( "110" );
  }
}

function TestCase(n, d, e, a)
{
  this.path = (typeof gTestPath == 'undefined') ?
    (gTestsuite + '/' + gTestsubsuite + '/' + gTestfile) :
    gTestPath;
  this.file = gTestfile;
  this.name = n;
  this.description = d;
  this.expect = e;
  this.actual = a;
  this.passed = getTestCaseResult(e, a);
  this.reason = '';
  this.bugnumber = typeof(BUGNUMER) != 'undefined' ? BUGNUMBER : '';
  this.type = (typeof window == 'undefined' ? 'shell' : 'browser');
  gTestcases[gTc++] = this;
}

TestCase.prototype.dump = function () {
  dump('\njstest: '      + this.path + ' ' +
       'bug: '         + this.bugnumber + ' ' +
       'result: '      + (this.passed ? 'PASSED':'FAILED') + ' ' +
       'type: '        + this.type + ' ' +
       'description: ' + toPrinted(this.description) + ' ' +
       'expected: '    + toPrinted(this.expect) + ' ' +
       'actual: '      + toPrinted(this.actual) + ' ' +
       'reason: '      + toPrinted(this.reason) + '\n');
};





function expectExitCode(n)
{
  print('--- NOTE: IN THIS TESTCASE, WE EXPECT EXIT CODE ' + n + ' ---');
}




function inSection(x)
{
  return SECT_PREFIX + x + SECT_SUFFIX;
}




function reportFailure (msg)
{
  var lines = msg.split ("\n");
  var l;
  var funcName = currentFunc();
  var prefix = (funcName) ? "[reported from " + funcName + "] ": "";
   
  for (var i=0; i<lines.length; i++)
    print (FAILED + prefix + lines[i]);
}




function printStatus (msg)
{




  msg = msg.toString();
  var lines = msg.split ("\n");
  var l;

  for (var i=0; i<lines.length; i++)
    print (STATUS + lines[i]);
}




function printBugNumber (num)
{
  BUGNUMBER = num;
  print ('BUGNUMBER: ' + num);
}

function toPrinted(value)
{
  if (typeof value == "xml") 
  {
    value = value.toXMLString();
  } 
  else 
  {
    value = String(value);
  }
  value = value.replace(/\\n/g, 'NL').replace(/\n/g, 'NL').replace(/\\r/g, 'CR');
  return value;
}






function reportCompare (expected, actual, description) {
  var expected_t = typeof expected;
  var actual_t = typeof actual;
  var output = "";
   
  if ((VERBOSE) && (typeof description != "undefined"))
    printStatus ("Comparing '" + description + "'");

  if (expected_t != actual_t)
  {
    output += "Type mismatch, expected type " + expected_t +
      ", actual type " + actual_t + " ";
  }
  else if (VERBOSE)
  {
    printStatus ("Expected type '" + expected_t + "' matched actual " +
                 "type '" + actual_t + "'");
  }

  if (expected != actual)
  {
    output += "Expected value '" + toPrinted(expected) +
      "', Actual value '" + toPrinted(actual) + "' ";
  }
  else if (VERBOSE)
  {
    printStatus ("Expected value '" + toPrinted(expected) +
                 "' matched actual value '" + toPrinted(actual) + "'");
  }

  if (typeof description == "undefined")
    description = '';
   
  var testcase = new TestCase(gTestfile, description, expected, actual);
  testcase.reason = output;
 
  if (testcase.passed)
  {
    print('PASSED! ' + description);
  }
  else
  {
    reportFailure (output);  
  }

  return testcase.passed;
}







function reportMatch (expectedRegExp, actual, description) {
  var expected_t = "string";
  var actual_t = typeof actual;
  var output = "";

  if ((VERBOSE) && (typeof description != "undefined"))
    printStatus ("Comparing '" + description + "'");

  if (expected_t != actual_t)
  {
    output += "Type mismatch, expected type " + expected_t +
      ", actual type " + actual_t + " ";
  }
  else if (VERBOSE)
  {
    printStatus ("Expected type '" + expected_t + "' matched actual " +
                 "type '" + actual_t + "'");
  }

  var matches = expectedRegExp.test(actual);
  if (!matches)
  {
    output += "Expected match to '" + toPrinted(expectedRegExp) +
      "', Actual value '" + toPrinted(actual) + "' ";
  }
  else if (VERBOSE)
  {
    printStatus ("Expected match to '" + toPrinted(expectedRegExp) +
                 "' matched actual value '" + toPrinted(actual) + "'");
  }

  if (typeof description == "undefined")
    description = '';

  var testcase = new TestCase(gTestfile, description, true, matches);
  testcase.reason = output;

  if (testcase.passed)
  {
    print('PASSED! ' + description);
  }
  else
  {
    reportFailure (output);
  }

  return testcase.passed;
}





function enterFunc (funcName)
{
  if (!funcName.match(/\(\)$/))
    funcName += "()";

  callStack.push(funcName);
}





function exitFunc (funcName)
{
  var lastFunc = callStack.pop();
   
  if (funcName)
  {
    if (!funcName.match(/\(\)$/))
      funcName += "()";

    if (lastFunc != funcName)
      reportCompare(funcName, lastFunc, "Test driver failure wrong exit function ");
  }
}




function currentFunc()
{
  return callStack[callStack.length - 1];
}






function BigO(data)
{
  var order = 0;
  var origLength = data.X.length;

  while (data.X.length > 2)
  {
    var lr = new LinearRegression(data);
    if (lr.b > 1e-6)
    {
      
      
      order++;
    }

    if (lr.r > 0.98 || lr.Syx < 1 || lr.b < 1e-6)
    {
      
      
      
      break;
    }
    data = dataDeriv(data);
  }

  if (2 == origLength - order)
  {
    order = Number.POSITIVE_INFINITY;
  }
  return order;

  function LinearRegression(data)
  {
    






    var i;

    if (data.X.length != data.Y.length)
    {
      throw 'LinearRegression: data point length mismatch';
    }
    if (data.X.length < 3)
    {
      throw 'LinearRegression: data point length < 2';
    }
    var n = data.X.length;
    var X = data.X;
    var Y = data.Y;

    this.Xavg = 0;
    this.Yavg = 0;

    var SUM_X  = 0;
    var SUM_XY = 0;
    var SUM_XX = 0;
    var SUM_Y  = 0;
    var SUM_YY = 0;

    for (i = 0; i < n; i++)
    {
      SUM_X  += X[i];
      SUM_XY += X[i]*Y[i];
      SUM_XX += X[i]*X[i];
      SUM_Y  += Y[i];
      SUM_YY += Y[i]*Y[i];
    }

    this.b = (n * SUM_XY - SUM_X * SUM_Y)/(n * SUM_XX - SUM_X * SUM_X);
    this.a = (SUM_Y - this.b * SUM_X)/n;

    this.Xavg = SUM_X/n;
    this.Yavg = SUM_Y/n;

    var SUM_Ydiff2 = 0;
    var SUM_Xdiff2 = 0;
    var SUM_XdiffYdiff = 0;

    for (i = 0; i < n; i++)
    {
      var Ydiff = Y[i] - this.Yavg;
      var Xdiff = X[i] - this.Xavg;
       
      SUM_Ydiff2 += Ydiff * Ydiff;
      SUM_Xdiff2 += Xdiff * Xdiff;
      SUM_XdiffYdiff += Xdiff * Ydiff;
    }

    var Syx2 = (SUM_Ydiff2 - Math.pow(SUM_XdiffYdiff/SUM_Xdiff2, 2))/(n - 2);
    var r2   = Math.pow((n*SUM_XY - SUM_X * SUM_Y), 2) /
      ((n*SUM_XX - SUM_X*SUM_X)*(n*SUM_YY-SUM_Y*SUM_Y));

    this.Syx = Math.sqrt(Syx2);
    this.r = Math.sqrt(r2);

  }

  function dataDeriv(data)
  {
    if (data.X.length != data.Y.length)
    {
      throw 'length mismatch';
    }
    var length = data.X.length;

    if (length < 2)
    {
      throw 'length ' + length + ' must be >= 2';
    }
    var X = data.X;
    var Y = data.Y;

    var deriv = {X: [], Y: [] };

    for (var i = 0; i < length - 1; i++)
    {
      deriv.X[i] = (X[i] + X[i+1])/2;
      deriv.Y[i] = (Y[i+1] - Y[i])/(X[i+1] - X[i]);
    } 
    return deriv;
  }

  return 0;
}

function compareSource(expect, actual, summary)
{
  
  var expectP = expect.
    replace(/([(){},.:\[\]])/mg, ' $1 ').
    replace(/(\w+)/mg, ' $1 ').
    replace(/<(\/)? (\w+) (\/)?>/mg, '<$1$2$3>').
    replace(/\s+/mg, ' ').
    replace(/new (\w+)\s*\(\s*\)/mg, 'new $1');

  var actualP = actual.
    replace(/([(){},.:\[\]])/mg, ' $1 ').
    replace(/(\w+)/mg, ' $1 ').
    replace(/<(\/)? (\w+) (\/)?>/mg, '<$1$2$3>').
    replace(/\s+/mg, ' ').
    replace(/new (\w+)\s*\(\s*\)/mg, 'new $1');

  print('expect:\n' + expectP);
  print('actual:\n' + actualP);

  reportCompare(expectP, actualP, summary);

  
  try
  {
    var expectCompile = 'No Error';
    var actualCompile;

    eval(expect);
    try
    {
      eval(actual);
      actualCompile = 'No Error';
    }
    catch(ex1)
    {
      actualCompile = ex1 + '';
    }
    reportCompare(expectCompile, actualCompile,
                  summary + ': compile actual');
  }
  catch(ex)
  {
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

function getTestCaseResult(expected, actual)
{
  var expected_t = typeof expected;
  var actual_t = typeof actual;
  var passed = true;
 
  
  
  if ( actual != actual ) 
  {
    if ( actual_t == "object" ) 
    {
      actual = "NaN object";
    } 
    else 
    {
      actual = "NaN number";
    }
  }
  if ( expected != expected ) 
  {
    if ( expected_t == "object" ) 
    {
      expected = "NaN object";
    } 
    else 
    {
      expected = "NaN number";
    }
  }

  if (expected_t != actual_t)
  {
    passed = false;
  }
  else if (expected != actual)
  {
    if (expected_t != 'number' || (Math.abs(actual - expected) > 1E-10))
    {
      passed = false;
    }
  }
 
  return passed;
}

if (typeof dump == 'undefined')
{
  if (typeof window == 'undefined' &&
      typeof print == 'function')
  {
    dump = print;
  }
  else
  {
    dump = (function () {});
  }
}

function test() {
  for ( gTc=0; gTc < gTestcases.length; gTc++ ) {
    
    try
    {
      gTestcases[gTc].passed = writeTestCaseResult(
        gTestcases[gTc].expect,
        gTestcases[gTc].actual,
        gTestcases[gTc].description +" = "+ gTestcases[gTc].actual );
      gTestcases[gTc].reason += ( gTestcases[gTc].passed ) ? "" : "wrong value ";
    }
    catch(e)
    {
      print('test(): empty testcase for gTc = ' + gTc + ' ' + e);
    }
  }
  stopTest();
  return ( gTestcases );
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
  for ( var i = 0; i < gTestcases.length; i++ ) {
    if ( ! gTestcases[i].passed ) {
      print( gTestcases[i].description + " = " +gTestcases[i].actual +
             " expected: " + gTestcases[i].expect );
    }
  }
}

function jsTestDriverEnd()
{
  
  
  
  
  
  
  

  if (gDelayTestDriverEnd)
  {
    return;
  }

  try
  {
    optionsReset();
  }
  catch(ex)
  {
    dump('jsTestDriverEnd ' + ex);
  }

  for (var i = 0; i < gTestcases.length; i++)
  {
    gTestcases[i].dump();
  }

}




function inRhino()
{
  return (typeof defineClass == "function");
}



function GetContext() {
  return Packages.com.netscape.javascript.Context.getCurrentContext();
}
function OptLevel( i ) {
  i = Number(i);
  var cx = GetContext();
  cx.setOptimizationLevel(i);
}



