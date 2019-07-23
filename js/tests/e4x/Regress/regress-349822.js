





































var bug = 349822;
var summary = 'decompilation of x.@[2]';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

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

compareSource(1, expect, actual);

END();
