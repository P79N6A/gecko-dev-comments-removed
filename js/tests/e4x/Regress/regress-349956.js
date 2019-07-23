





































var bug = 349956;
var summary = 'decompilation of <x/>.@*';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var f;
var g;

f = function () { <x/>.@* };
g = eval('(' + f + ')');

expect = f + '';
actual = g + '';

compareSource(1, expect, actual);

END();
