




































var gTestfile = 'regress-353079.js';

var BUGNUMBER = 353079;
var summary = 'Do not Assert op == JSOP_LEAVEBLOCKEXPR... with WAY_TOO_MUCH_GC';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  for (let a in [1]) let (x) {
    for (let y in ((function(id2) { return id2; })( '' ))) { } }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
