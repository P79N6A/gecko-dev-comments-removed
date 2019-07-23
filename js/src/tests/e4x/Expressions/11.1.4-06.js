




































gTestfile = '11.1.4-06.js';

var summary = "11.1.4 - ]] should be allowed in CDATA Section";
var BUGNUMBER = 313929;
var actual = 'No error';
var expect = 'No error';

printBugNumber(BUGNUMBER);
START(summary);

try
{
    actual = XML("<x><![CDATA[ ]] ]]></x>").toString();
}
catch(e)
{
    actual = e + '';
}

expect = (<x> ]] </x>).toString();

TEST(1, expect, actual);

END();
