




































var gTestfile = 'regress-338804-03.js';

var BUGNUMBER = 338804;
var summary = 'GC hazards in constructor functions';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

if (typeof Script != 'undefined')
{
  Script({ toString: fillHeap });
}
RegExp({ toString: fillHeap });

function fillHeap() {
  if (typeof gc == 'function') gc();
  var x = 1, tmp;
  for (var i = 0; i != 50000; ++i) {
    tmp = x / 3;
  }
}
 
reportCompare(expect, actual, summary);
