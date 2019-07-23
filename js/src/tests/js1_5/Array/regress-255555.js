





































var gTestfile = 'regress-255555.js';

var BUGNUMBER = 255555;
var summary = 'Array.prototype.sort(comparefn) never passes undefined to comparefn';
var actual = 'not undefined';
var expect = 'not undefined';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
function comparefn(a,b)
{
  if (typeof a == 'undefined')
  {
    actual = 'undefined';
    return 1;
  }
  if (typeof b == 'undefined')
  {
    actual = 'undefined';
    return -1;
  }
  return a - b;
}

var arry = [ 1, 2, undefined ].sort(comparefn)

  reportCompare(expect, actual, summary);
