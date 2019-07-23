




































var gTestfile = 'regress-425360.js';

var BUGNUMBER = 425360;
var summary = 'Do not assert: !cx->throwing';
var actual = 'No Crash';
var expect = 'No Crash';

function finishtest()
{
  gDelayTestDriverEnd = false;
  reportCompare(expect, actual, summary);
  jsTestDriverEnd();
}

function throwBlah()
{
  throw 'blah';
}

printBugNumber(BUGNUMBER);
printStatus (summary);
 
if (typeof window == 'undefined')
{
  expect = actual = 'Not tested. Requires browser.';
  reportCompare(expect, actual, summary);
}
else
{
  gDelayTestDriverEnd = true;
  window.onerror = null;
  setTimeout('finishtest()', 1000);
  window.onload = (function () { setInterval('throwBlah()', 0); });
  setInterval('foo(', 0);
}


