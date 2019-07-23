




































var gTestfile = 'regress-382503.js';

var BUGNUMBER = 382503;
var summary = 'Do not assert: with prototype=regexp';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
function f(x)
{
  prototype = /a/;

  if (x) {
    return /b/;
    return /c/;
  } else {
    return /d/;
  }
}

void f(false);

reportCompare(expect, actual, summary);
