






































var summary = 'Infinite recursion should throw catchable exception';
var BUGNUMBER = 394941;
var actual = '';
var expect = /InternalError: too much recursion/;

expectExitCode(0);
expectExitCode(5);







printBugNumber(BUGNUMBER);
START(summary);

try 
{
    function f() { var z = <x><y/></x>; f(); }
    f();
} 
catch(ex) 
{
    actual = ex + '';
    print("Caught: " + ex);
}

reportMatch(expect, actual);

END();
