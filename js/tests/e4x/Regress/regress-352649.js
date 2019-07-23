





































var bug = 352649;
var summary = 'decompilation of e4x literal after |if| block';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var f = function() { if(g) p; (<x/>.i) }
expect = 'function() { if(g) {p;} <x/>.i; }';
actual = f + '';
compareSource(1, expect, actual)

END();
