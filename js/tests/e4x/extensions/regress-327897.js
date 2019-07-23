





































START("Crash in js_GetStringBytes");

var bug = 327897;
var summary = 'Crash in js_GetStringBytes';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);
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
