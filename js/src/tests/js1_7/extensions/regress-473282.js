






































var BUGNUMBER = 473282;
var summary = 'Do not assert: thing';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
this.watch("b", "".substring);
__defineGetter__("a", gc);
for each (b in [this, null, null]);
a;

reportCompare(expect, actual, summary);
