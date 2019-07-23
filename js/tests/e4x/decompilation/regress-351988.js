





































gTestfile = 'regress-351988.js';

var summary = 'decompilation of XMLPI object initializer';
var BUGNUMBER = 351988;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var f;
f = function() { var y = <?foo bar?>; }
actual = f + '';
expect = 'function () {\n    var y = <?foo bar?>;\n}';

compareSource(expect, actual, inSection(1) + summary);

END();
