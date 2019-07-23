





































var gTestfile = 'regress-245148.js';

var BUGNUMBER = 245148;
var summary = '[null].toSource() == "[null]"';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

if (typeof Array.prototype.toSource != 'undefined')
{
  expect = '[null]';
  actual = [null].toSource();

  reportCompare(expect, actual, summary);
}
