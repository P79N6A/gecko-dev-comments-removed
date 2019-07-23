












































var gTestfile = 'regress-77578-001.js';
var UBound = 0;
var BUGNUMBER = 77578;
var summary = 'Testing eval scope inside a function';
var cnEquals = '=';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];



var JS_VER = [100, 110, 120, 130, 140, 150];


var i = 999;
var j = 999;
var k = 999;



test();



function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  
  for (var n=0; n!=JS_VER.length; n++)
  {
    testA(JS_VER[n]);
  }
  for (var n=0; n!=JS_VER.length; n++)
  {
    testB(JS_VER[n]);
  }
  for (var n=0; n!=JS_VER.length; n++)
  {
    testC(JS_VER[n]);
  }


  
  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}


function testA(ver)
{
  
  if (typeof version == 'function')
  {
    version(ver);
  }

  
  var sTestScript = "";

  
  sTestScript += "status = 'Section A of test; JS ' + ver/100;";
  sTestScript += "var i=1;";
  sTestScript += "actual = eval('i');";
  sTestScript += "expect = 1;";
  sTestScript += "captureThis('i');";

  eval(sTestScript);
}


function testB(ver)
{
  
  if (typeof version == 'function')
  {
    version(ver);
  }

  
  var sTestScript = "";

  
  sTestScript += "status = 'Section B of test; JS ' + ver/100;";
  sTestScript += "for(var j=1; j<2; j++)";
  sTestScript += "{";
  sTestScript += "  actual = eval('j');";
  sTestScript += "};";
  sTestScript += "expect = 1;";
  sTestScript += "captureThis('j');";

  eval(sTestScript);
}


function testC(ver)
{
  
  if (typeof version == 'function')
  {
    version(ver);
  }

  
  var sTestScript = "";

  
  sTestScript += "status = 'Section C of test; JS ' + ver/100;";
  sTestScript += "try";
  sTestScript += "{";
  sTestScript += "  var k=1;";
  sTestScript += "  actual = eval('k');";
  sTestScript += "}";
  sTestScript += "catch(e)";
  sTestScript += "{";
  sTestScript += "};";
  sTestScript += "expect = 1;";
  sTestScript += "captureThis('k');";

  eval(sTestScript);
}


function captureThis(varName)
{
  statusitems[UBound] = status;
  actualvalues[UBound] = varName + cnEquals + actual;
  expectedvalues[UBound] = varName + cnEquals + expect;
  UBound++;
}
