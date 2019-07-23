




































START("11.1.4 - ]] should be allowed in CDATA Section");

var bug = 313929;
var summary = ']] should be allowed in CDATA Section';
var actual = 'No error';
var expect = 'No error';

printBugNumber (bug);
printStatus (summary);

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
