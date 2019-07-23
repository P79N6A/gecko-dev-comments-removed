





































var bug = 355478;
var summary = 'Do not crash with hasOwnProperty';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

expect = 'TypeError: XML.prototype.hasOwnProperty called on incompatible Object';
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
