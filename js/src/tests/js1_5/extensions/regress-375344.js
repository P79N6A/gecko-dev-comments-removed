




































var gTestfile = 'regress-375344.js';

var BUGNUMBER = 375344;
var summary = 'accessing prototype of DOM objects should throw catchable error';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

if (typeof HTMLElement != 'undefined')
{
  expect = /Exception... "Illegal operation on WrappedNative prototype object"/;
  try 
  {
    print(HTMLElement.prototype.nodeName );
  }
  catch(ex) 
  {
    actual = ex + '';
    print(actual);
  }
  reportMatch(expect, actual, summary);
}
else
{
  expect = actual = 'Test can only run in a Gecko 1.9 browser or later.';
  print(actual);
  reportCompare(expect, actual, summary);
}
