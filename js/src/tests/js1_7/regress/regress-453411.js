




































var gTestfile = 'regress-453411.js';

var BUGNUMBER = 453411;
var summary = 'Do not assert with JIT: !cx->executingTrace|!tm->onTrace';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  for (var i in (function(){ for (var j=0;j<4;++j) { yield ""; } })());

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
