





































var bug = 355101;
var summary = 'XML Filtering predicate operator';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var f = function () { return i.(true); }
expect = 'function () { return i.(true); }';
actual = f + '';
compareSource(1, expect, actual);

END();
