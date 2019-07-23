




































var gTestfile = 'regress-452703.js';

var BUGNUMBER = 452703;
var summary = 'Do not assert with JIT: rmask(rr)&FpRegs';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

(function() { for(let y in [0,1,2,3,4]) y = NaN; })();

jit(false);

reportCompare(expect, actual, summary);
