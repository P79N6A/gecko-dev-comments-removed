










































if (typeof version != 'undefined')
{
  version(0);
}

var STATUS = "STATUS: ";
var VERBOSE = false;
var SECT_PREFIX = 'Section ';
var SECT_SUFFIX = ' of test - ';
var callStack = new Array();


if (typeof __defineGetter__ == 'function' && typeof __defineSetter__ == 'function')
{
  __defineGetter__('gTestfile', (function () { return this._gTestfile; }));
  __defineSetter__('gTestfile', (function (v) { print('begin test: ' + gTestsuite + '/' + gTestsubsuite + '/' + v); this._gTestfile = v; }));
}

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




var GLOBAL = this + '';
var PASSED = " PASSED! ";
var FAILED = " FAILED! ";

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

gFailureExpected = false;

TestCase.prototype.dump = function () {
  
  
  if (typeof document != "object" ||
      !document.location.href.match(/jsreftest.html/))
  {
    dump('\njstest: ' + this.path + ' ' +
         'bug: '         + this.bugnumber + ' ' +
         'result: '      + (this.passed ? 'PASSED':'FAILED') + ' ' +
         'type: '        + this.type + ' ' +
         'description: ' + toPrinted(this.description) + ' ' +


         'reason: '      + toPrinted(this.reason) + '\n');
  }
};

TestCase.prototype.testPassed = (function TestCase_testPassed() { return this.passed; });
TestCase.prototype.testFailed = (function TestCase_testFailed() { return !this.passed; });
TestCase.prototype.testDescription = (function TestCase_testDescription() { return this.description + ' ' + this.reason; });

function getTestCases()
{
  return gTestcases;
}





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
  value = value.replace(/\\n/g, 'NL')
               .replace(/\n/g, 'NL')
               .replace(/\\r/g, 'CR')
               .replace(/[^\x20-\x7E]+/g, escapeString);
  return value;
}

function escapeString (str)
{
  var a, b, c, d;
  var len = str.length;
  var result = "";
  var digits = ["0", "1", "2", "3", "4", "5", "6", "7",
                "8", "9", "A", "B", "C", "D", "E", "F"];

  for (var i=0; i<len; i++)
  {
    var ch = str.charCodeAt(i);

    a = digits[ch & 0xf];
    ch >>= 4;
    b = digits[ch & 0xf];
    ch >>= 4;

    if (ch)
    {
      c = digits[ch & 0xf];
      ch >>= 4;
      d = digits[ch & 0xf];

      result += "\\u" + d + c + b + a;
    }
    else
    {
      result += "\\x" + b + a;
    }
  }

  return result;
}










if (typeof assertEq == 'undefined')
{
  var assertEq =
    function (actual, expected, message)
    {
      function SameValue(v1, v2)
      {
        if (v1 === 0 && v2 === 0)
          return 1 / v1 === 1 / v2;
        if (v1 !== v1 && v2 !== v2)
          return true;
        return v1 === v2;
      }
      if (!SameValue(actual, expected))
      {
        throw new TypeError('Assertion failed: got "' + actual + '", expected "' + expected +
                            (message ? ": " + message : ""));
      }
    };
}






function reportCompare (expected, actual, description) {
  var expected_t = typeof expected;
  var actual_t = typeof actual;
  var output = "";

  if (typeof description == "undefined")
  {
    description = '';
  }
  else if (VERBOSE)
  {
    printStatus ("Comparing '" + description + "'");
  }

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

  var testcase = new TestCase(gTestfile, description, expected, actual);
  testcase.reason = output;

  if (testcase.passed)
  {
    print(PASSED + description);
  }
  else
  {
    reportFailure (description + " : " + output);
  }

  return testcase.passed;
}







function reportMatch (expectedRegExp, actual, description) {
  var expected_t = "string";
  var actual_t = typeof actual;
  var output = "";

  if (typeof description == "undefined")
  {
    description = '';
  }
  else if (VERBOSE)
  {
    printStatus ("Comparing '" + description + "'");
  }

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

  var testcase = new TestCase(gTestfile, description, true, matches);
  testcase.reason = output;

  if (testcase.passed)
  {
    print(PASSED + description);
  }
  else
  {
    reportFailure (description + " : " + output);
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

  try
  {
    optionsClear();

    
    for (optionName in options.initvalues)
    {
      options(optionName);
    }
  }
  catch(ex)
  {
    print('optionsReset: caught ' + ex);
  }

}

if (typeof options == 'function')
{
  optionsInit();
  optionsClear();
}

function getTestCaseResult(expected, actual)
{
  if (typeof expected != typeof actual)
    return false;
  if (typeof expected != 'number')
    
    return actual == expected;

  
  
  if (actual != actual)
    return expected != expected;
  if (expected != expected)
    return false;

  
  if (actual != expected)
    return Math.abs(actual - expected) <= 1E-10;

  
  
  
  
  
  
  return true;
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
  var s = ( passed ? PASSED : FAILED ) + string + ' expected: ' + expect;
  print(s);
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

function jit(on)
{
  if (on && !options().match(/jit/))
  {
    options('jit');
  }
  else if (!on && options().match(/jit/))
  {
    options('jit');
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



