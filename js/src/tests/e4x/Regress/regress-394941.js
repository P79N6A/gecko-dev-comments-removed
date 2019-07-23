





































gTestfile = 'regress-394941.js';

var summary = 'Infinite recursion should throw catchable exception';
var BUGNUMBER = 394941;
var actual = '';
var expect = /InternalError: (script stack space quota is exhausted|too much recursion)/;







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
