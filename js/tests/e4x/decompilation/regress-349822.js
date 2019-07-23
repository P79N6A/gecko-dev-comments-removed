





































gTestfile = 'regress-349822.js';

var BUGNUMBER = 349822;
var summary = 'decompilation of x.@[2]';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

expect = 'function () {\n    return x.@[2];\n}';

try
{
    var f = eval('(function () { return x.@[2]; })');
    actual = f + '';
}
catch(ex)
{
    actual = ex + '';
}

compareSource(expect, actual, inSection(1) + summary);

END();
