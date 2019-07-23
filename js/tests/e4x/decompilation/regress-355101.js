





































gTestfile = 'regress-355101.js';

var BUGNUMBER = 355101;
var summary = 'XML Filtering predicate operator';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var f = function () { return i.(true); }
expect = 'function () { return i.(true); }';
actual = f + '';
compareSource(expect, actual, inSection(1) + summary);

END();
