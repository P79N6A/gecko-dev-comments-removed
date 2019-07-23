





































gTestfile = 'regress-349815.js';

var BUGNUMBER = 349815;
var summary = 'decompilation of parameterized e4x xmllist literal';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var f = function (tag) { return <><{tag}></{tag}></>; }

expect = 'function (tag) {\n    return <><{tag}></{tag}></>;\n}';
actual = f + '';

compareSource(expect, actual, inSection(1) + summary);

END();
