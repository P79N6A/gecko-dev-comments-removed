





var BUGNUMBER = 369696;
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
  assertEq(q.toSource(), "[\"\", , []]", "wrong string");

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
