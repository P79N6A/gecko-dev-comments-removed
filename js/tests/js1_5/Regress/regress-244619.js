





































var gTestfile = 'regress-244619.js';

var BUGNUMBER = 244619;
var summary = 'Don\'t Crash';
var actual = 'Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

function f1()
{
  var o = new Object();
  eval.call(o, "var a = 'vodka'"); 
}


try
{
  f1();
}
catch(e)
{
}

actual = 'No Crash';
 
reportCompare(expect, actual, summary);
