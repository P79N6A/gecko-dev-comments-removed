





































gTestfile = 'regress-344455.js';

var summary = "Error - tag name mismatch error message should include tag name";
var BUGNUMBER = 344455;
var actual = '';
var expect = 'SyntaxError: XML tag name mismatch (expected foo)';

printBugNumber(BUGNUMBER);
START(summary);

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
