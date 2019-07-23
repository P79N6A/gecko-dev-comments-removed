





































gTestfile = 'regress-450871-01.js';

var summary = 'Do not crash: __proto__ = <x/>; <x/>.lastIndexOf(this, false)';
var BUGNUMBER = 450871;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

if (typeof window == 'object')
{
    actual = expect = 'Test skipped for browser based tests due destruction of the prototype';
}
else
{
    try
    {
        __proto__ = <x/>; 
        <x/>.lastIndexOf(this, false);
    }
    catch(ex)
    {
    }
}

TEST(1, expect, actual);

END();
