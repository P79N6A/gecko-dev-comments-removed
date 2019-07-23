





































var bug = 350351;
var summary = "decompilation of function (){ return (@['a'])=='b'}";
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var f;

f = function (){ return (@['a'])=='b'}
expect = 'function () {\n    return @["a"] == "b";\n}';
actual = f + '';
compareSource(1, expect, actual);

f = function (){ return (@['a']).toXMLString() }
expect = 'function () {\n    return @["a"].toXMLString();\n}';
actual = f + '';
compareSource(2, expect, actual);

END();
