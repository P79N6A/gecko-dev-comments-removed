




































var gTestfile = 'regress-465337.js';

var BUGNUMBER = 465337;
var summary = 'Do not assert: (m != JSVAL_INT) || isInt32(*vp)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true); 
  var out = [];
  for (let j = 0; j < 5; ++j) { out.push(6 - ((void 0) ^ 0x80000005)); }
  print(uneval(out));
  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
