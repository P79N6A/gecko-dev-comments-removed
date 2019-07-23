




































var gTestfile = 'regress-338121-01.js';

var BUGNUMBER = 338121;
var summary = 'Issues with JS_ARENA_ALLOCATE_CAST';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

var fe="v";

for (i=0; i<25; i++)
  fe += fe;

var fu=new Function(
  fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe,
  fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe, fe,
  "done"
  );

print('Done');

reportCompare(expect, actual, summary);
