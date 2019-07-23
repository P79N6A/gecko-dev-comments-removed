





































gTestfile = 'regress-350531.js';

var BUGNUMBER = 350531;
var summary = "decompilation of function (){ return (@['a'])=='b'}";
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var f;

f = function (){ return (@['a'])=='b'}
expect = 'function () {\n    return @["a"] == "b";\n}';
actual = f + '';
compareSource(expect, actual, inSection(1) + summary);

f = function (){ return (@['a']).toXMLString() }
expect = 'function () {\n    return @["a"].toXMLString();\n}';
actual = f + '';
compareSource(expect, actual, inSection(2) + summary);

END();
