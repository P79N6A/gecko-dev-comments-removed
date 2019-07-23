





































var bug = 351988;
var summary = 'decompilation of XMLPI object initializer';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var f;
f = function() { var y = <?foo bar?>; }
actual = f + '';
expect = 'function () {\n    var y = <?foo bar?>;\n}';

compareSource(1, expect, actual);

END();
