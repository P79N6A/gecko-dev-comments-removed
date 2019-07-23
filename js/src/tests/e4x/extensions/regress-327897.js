





































gTestfile = 'regress-327897.js';

var summary = "Crash in js_GetStringBytes";
var BUGNUMBER = 327897;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);
printStatus('This test runs only in the browser');

if (typeof XMLSerializer != 'undefined')
{
    try
    {
        var a = XMLSerializer;
        a.foo = (function(){}).apply;
        a.__proto__ = <x/>;
        a.foo();
    }
    catch(ex)
    {
        printStatus(ex + '');
    }
}
TEST(1, expect, actual);

END();
