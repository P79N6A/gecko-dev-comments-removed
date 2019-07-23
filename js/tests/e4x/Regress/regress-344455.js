





































START("Error - tag name mismatch error message should include tag name");

var bug = 344455;
var summary = 'Error - tag name mismatch error message should include tag name';
var actual = '';
var expect = 'SyntaxError: XML tag name mismatch (expected foo)';

printBugNumber (bug);
printStatus (summary);

try
{
    eval('x = <foo></bar>;');
}
catch(ex)
{
    actual = ex + '';
}

TEST(1, expect, actual);

END();
