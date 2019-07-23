






































var gTestfile = 'regress-255245.js';

var BUGNUMBER = 255245;
var summary = 'Function.prototype.toSource/.toString show "setrval" instead of "return"';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

function f() {
  try {
  } catch (e) {
    return false;
  }
  finally {
  }
}

if (typeof f.toSource != 'undefined')
{
  expect = -1;
  actual = f.toSource().indexOf('setrval');

  reportCompare(expect, actual, summary);
}
