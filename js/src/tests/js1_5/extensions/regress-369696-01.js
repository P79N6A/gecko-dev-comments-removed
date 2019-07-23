




































var gTestfile = 'regress-369696-01.js';

var BUGNUMBER = 396696;
var summary = 'Do not assert: map->depth > 0" in js_LeaveSharpObject';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  q = [];
  q.__defineGetter__("0", q.toString);
  q[2] = q;
  q.toSource();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
