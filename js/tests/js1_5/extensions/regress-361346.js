




































var gTestfile = 'regress-361346.js';

var BUGNUMBER = 361346;
var summary = 'Crash with setter, watch, GC';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
expect = actual = 'No Crash';

this.x setter= new Function;
this.watch('x', function(){});
gc();
x = {};

reportCompare(expect, actual, summary);
