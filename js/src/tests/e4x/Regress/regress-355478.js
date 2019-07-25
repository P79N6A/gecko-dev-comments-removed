







var BUGNUMBER = 355478;
var summary = 'Do not crash with hasOwnProperty';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

expect = 'TypeError: (intermediate value).hasOwnProperty is not a constructor';
actual = '';

try
{
    new <x/>.hasOwnProperty("y");
}
catch(ex)
{
    actual = ex + '';
}

TEST(1, expect, actual);
END();
