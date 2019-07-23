




































var gTestfile = 'regress-474771-01.js';

var BUGNUMBER = 474771;
var summary = 'TM: do not assert: jumpTable == interruptJumpTable';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  var o = {};
  o.__defineSetter__('x', function(){});
  for (let j = 0; j < 4; ++j) o.x = 3;

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
