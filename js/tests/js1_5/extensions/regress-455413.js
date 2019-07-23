




































var gTestfile = 'regress-455413.js';

var BUGNUMBER = 455413;
var summary = 'Do not assert with JIT: (m != JSVAL_INT) || isInt32(*vp)';
var actual = 'No Crash';
var expect = 'No Crash';



test();


printBugNumber(BUGNUMBER);
printStatus (summary);
 
jit(true);

this.watch('x', Math.pow); (function() { for(var j=0;j<4;++j){x=1;} })();

jit(false);

reportCompare(expect, actual, summary);
