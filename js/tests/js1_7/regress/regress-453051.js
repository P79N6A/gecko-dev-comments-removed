




































var gTestfile = 'regress-453051.js';

var BUGNUMBER = 453051;
var summary = 'Do not assert with JIT: !(((*pc == JSOP_GOTO) || (*pc == JSOP_GOTOX)) && (exitType != LOOP_EXIT))';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  for (var p in this){} for (let a in [5,6,7]) for (var b=0;b<1;++b) break;

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
