





































gTestfile = 'regress-350226.js';

var BUGNUMBER = 350226;
var summary = 'decompilation of <x/>.@[*]';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var f = function () { <x/>[@[*]]; }
expect = 'function () {\n    <x/>[@[*]];\n}';
actual = f + '';

compareSource(expect, actual, inSection(1) + summary);

END();
