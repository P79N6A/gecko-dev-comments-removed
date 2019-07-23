





































var gTestfile = 'regress-238881.js';

var BUGNUMBER = 238881;
var summary = 'const propagation for switch too aggressive';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

const C=42;
function f(C,x)
{
  switch(x)
  {
  case C:
    return 1;
  default:
    return 0;
  }
}

actual = f(44,42);
expect = 0;
 
reportCompare(expect, actual, summary);
