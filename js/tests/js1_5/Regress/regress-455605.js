




































var gTestfile = 'regress-455605.js';

var BUGNUMBER = 455605;
var summary = 'Do not assert with JIT: "need a way to EOT now, since this is trace end": 0';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  for (var j = 0; j < 4; ++j) { switch(0/0) { } }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
