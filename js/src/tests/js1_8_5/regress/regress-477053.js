






var summary = 'Do not assert: v.isString()';
var BUGNUMBER = 477053;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);

try
{
    function f() { eval("with(arguments)throw [];"); }
    f();
}
catch(ex)
{
}

reportCompare(expect, actual);

