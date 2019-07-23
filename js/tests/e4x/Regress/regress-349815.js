





































var bug = 349815;
var summary = 'decompilation of parameterized e4x xmllist literal';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var f = function (tag) { return <><{tag}></{tag}></>; }

expect = 'function (tag) {\n    return <><{tag}></{tag}></>;\n}';
actual = f + '';

compareSource(1, expect, actual);

END();
