





var BUGNUMBER = 310993;
var summary = 'treat &lt;! as the start of a comment to end of line';
var actual = '';
var expect = '';


printBugNumber(BUGNUMBER);
printStatus (summary);

expect = 'foo';
actual = 'foo';

if (false) <!-- dumbdonkey -->
  actual = 'bar';

reportCompare(expect, actual, summary);
