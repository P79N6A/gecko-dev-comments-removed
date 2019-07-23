





































var gTestfile = 'regress-203278-3.js';

var BUGNUMBER = 203278;
var summary = 'Don\'t crash in recursive js_MarkGCThing';
var actual = 'FAIL';
var expect = 'PASS';

printBugNumber(BUGNUMBER);
printStatus (summary);




var a = new Array(1000 * 100);

var i = a.length;
while (i-- != 0)
{
  a[i] = {};
}




for (i = 0; i != 50*1000; ++i)
{
  a = [a, a.concat()];
}

if (typeof gc == 'function')
{
  gc();
}

actual = 'PASS';

reportCompare(expect, actual, summary);

