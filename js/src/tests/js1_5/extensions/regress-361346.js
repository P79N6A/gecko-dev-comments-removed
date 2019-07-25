





































var BUGNUMBER = 361346;
var summary = 'Crash with setter, watch, GC';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
expect = actual = 'No Crash';

Object.defineProperty(this, "x", { set: new Function, enumerable: true, configurable: true });
this.watch('x', function(){});
gc();
x = {};

reportCompare(expect, actual, summary);
