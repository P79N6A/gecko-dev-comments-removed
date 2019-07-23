





































gTestfile = 'regress-352649.js';

var BUGNUMBER = 352649;
var summary = 'decompilation of e4x literal after |if| block';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var f = function() { if(g) p; (<x/>.i) }
expect = 'function() { if(g) {p;} <x/>.i; }';
actual = f + '';
compareSource(expect, actual, inSection(1) + summary);

END();
