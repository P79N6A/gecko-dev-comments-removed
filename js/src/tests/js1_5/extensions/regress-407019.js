




































var gTestfile = 'regress-407019.js';

var BUGNUMBER = 407019;
var summary = 'Do not assert: !JS_IsExceptionPending(cx) - Browser only';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

if (typeof window != 'undefined' && typeof window.Option == 'function' && '__proto__' in window.Option)
{
  try
  {
    expect = /Illegal operation on WrappedNative prototype object/;
    window.Option("u", window.Option.__proto__);
    for (p in document) { }
    actual = 'No Error';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportMatch(expect, actual, summary);
}
else
{
  expect = actual = 'Test can only run in a Gecko 1.9 browser or later.';
  print(actual);
  reportCompare(expect, actual, summary);
}


