






































gTestfile = 'regress-426520.js';

var summary = 'Do not crash @ ParseXMLSource';
var BUGNUMBER = 426520;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

undefined = {};

try
{
    with (this) {
        throw <x/>;
    }
}
catch(ex)
{
}

TEST(1, expect, actual);

END();
