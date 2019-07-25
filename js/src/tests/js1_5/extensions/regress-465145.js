





































var BUGNUMBER = 465145;
var summary = 'Do not crash with watched defined setter';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
jit(true);

this.__defineSetter__("x", function(){});
this.watch("x", function(){});
y = this;
for (var z = 0; z < 2; ++z) { x = y };
this.__defineSetter__("x", function(){});
for (var z = 0; z < 2; ++z) { x = y };

jit(false);

reportCompare(expect, actual, summary);
