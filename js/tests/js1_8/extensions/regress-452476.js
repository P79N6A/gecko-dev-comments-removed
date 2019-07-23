




































var gTestfile = 'regress-452476.js';

var BUGNUMBER = 452476;
var summary = 'Do not assert with JIT: !cx->runningJittedCode';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);
 
for (var j = 0; j < 10; j++)
{
  for (var i = 0; i < j; ++i) 
    this["n" + i] = 1;

  __defineGetter__('w', (function(){})); 

  [1 for each (g in this) for each (t in /x/g)];
}

jit(false);

reportCompare(expect, actual, summary);
