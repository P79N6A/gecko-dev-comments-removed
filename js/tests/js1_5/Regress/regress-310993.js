




































var gTestfile = 'regress-310993.js';

var BUGNUMBER = 310993;
var summary = 'treat &lt;! as the start of a comment to end of line if not e4x';
var actual = '';
var expect = '';


printBugNumber(BUGNUMBER);
printStatus (summary);

expect = 'foo';
actual = 'foo';

if (false) <!-- dumbdonkey -->
  actual = 'bar';

reportCompare(expect, actual, summary);
