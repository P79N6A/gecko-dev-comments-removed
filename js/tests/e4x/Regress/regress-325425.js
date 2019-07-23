





































var bug = 325425;
var summary = 'jsxml.c: Bad assumptions about js_ConstructObject';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

try
{
    QName = function() { }; 
    <xml/>.elements("");
}
catch(ex)
{
    printStatus(ex + '');
}
TEST(1, expect, actual);

END();
