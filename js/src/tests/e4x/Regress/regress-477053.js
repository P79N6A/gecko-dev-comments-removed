





































gTestfile = 'regress-477053.js';

var summary = 'Do not assert: JSVAL_IS_STRING(v)';
var BUGNUMBER = 477053;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

try
{
    function f() { eval("with(arguments)throw <x/>;"); }
    f();
}
catch(ex)
{
}

TEST(1, expect, actual);

END();
