




































var gTestfile = 'regress-452372.js';

var BUGNUMBER = 452372;
var summary = 'Do not assert with JIT: entry->kpc == (jsbytecode*) atom';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  eval("(function() { for (var j = 0; j < 4; ++j) { /x/.__parent__; } })")();

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
