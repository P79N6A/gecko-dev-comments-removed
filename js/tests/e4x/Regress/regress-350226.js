





































var bug = 350226;
var summary = 'decompilation of <x/>.@[*]';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var f = function () { <x/>[@[*]]; }
expect = 'function () {\n    <x/>[@[*]];\n}';
actual = f + '';

compareSource(1, expect, actual);

END();
