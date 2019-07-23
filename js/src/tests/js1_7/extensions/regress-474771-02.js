




































var gTestfile = 'regress-474771-02.js';

var BUGNUMBER = 474771;
var summary = 'TM: do not assert: jumpTable == interruptJumpTable';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

this.__defineSetter__('x', function(){});
for (var j = 0; j < 5; ++j) { x = 3; }

jit(false);

reportCompare(expect, actual, summary);
