





































var gTestfile = 'regress-472508.js';

var BUGNUMBER = 472508;
var summary = 'TM: Do not crash @ TraceRecorder::emitTreeCall';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);
 
for (var x in this) { }
var a = [false, false, false];
a.__defineGetter__("q", function() { });
a.__defineGetter__("r", function() { });
for (var i = 0; i < 2; ++i) for each (var e in a) { }

jit(false);

reportCompare(expect, actual, summary);
