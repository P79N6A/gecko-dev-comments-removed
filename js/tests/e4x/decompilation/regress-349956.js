





































gTestfile = 'regress-349956.js';

var BUGNUMBER = 349956;
var summary = 'decompilation of <x/>.@*';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var f;
var g;

f = function () { <x/>.@* };
g = eval('(' + f + ')');

expect = f + '';
actual = g + '';

compareSource(expect, actual, inSection(1) + summary);

END();
