





































var gTestfile = 'regress-139316.js';

var BUGNUMBER = 139316;
var summary = 'Do not crash in js_ReportIsNotDefined()';
var actual = 'No Crash';
var expect = 'No Crash';


printBugNumber(BUGNUMBER);
printStatus (summary);

var str = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

function makeError ()
{
  try
  {
    foo();
  }
  catch (e)
  {
    return e;
  }
}


f = function ()
{
  var e = makeError (function c(){}, function  (){},
		     {a: 1}, null, undefined,
		     Number.MAX_VALUE, 0, new Number(1),
		     "hello world", str, new String ("newstring"),
		     true, new Boolean(0),
		     new Date());
  printStatus (e.stack);
}

  f();

 
reportCompare(expect, actual, summary);

