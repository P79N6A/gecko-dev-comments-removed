





































gTestfile = 'regress-378492.js';

var BUGNUMBER = 378492;
var summary = 'namespace_trace/qname_trace should check for null private, ' +
    'WAY_TOO_MUCH_GC';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

try
{
    eval('<x>');
}
catch(ex)
{
}

TEST(1, expect, actual);

END();
