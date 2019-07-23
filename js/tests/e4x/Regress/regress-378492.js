





































var bug = 378492;
var summary = 'namespace_trace/qname_trace should check for null private, ' +
    'WAY_TOO_MUCH_GC';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

try
{
    eval('<x>');
}
catch(ex)
{
}

TEST(1, expect, actual);

END();
