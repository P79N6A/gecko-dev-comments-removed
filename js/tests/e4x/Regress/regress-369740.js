





































gTestfile = 'regress-369740.js';

var BUGNUMBER = 369740;
var summary = 'generic code for function::';
var actual = 'No Exception';
var expect = 'No Exception';

printBugNumber(BUGNUMBER);
START(summary);

actual = expect = Math.function::sin + '';
TEST(1, expect, actual);

var x = <xml/>;
x.function::toString = function(){return "moo"};
actual = x + '';
expect = 'moo';
TEST(2, expect, actual);

x = <><a/><b/></>;

expect = 'test';
try
{
    with (x) {
        function::toString = function() { return "test"; }
    }

    actual = x + '';
}
catch(ex)
{
    actual = ex + '';
}
TEST(3, expect, actual);


expect = actual = 'No Crash';
const xhtmlNS = null;
this[xhtmlNS] = {};
TEST(4, expect, actual);


END();
