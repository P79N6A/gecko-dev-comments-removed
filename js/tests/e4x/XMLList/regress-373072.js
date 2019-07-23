





































var bug = 373072;
var summary = 'XML.prototype.namespace() does not check for xml list';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

try
{
    expect = 'TypeError: cannot call namespace method on an XML list with 0 elements';
    XML.prototype.function::namespace.call(new XMLList());
}
catch(ex)
{
    actual = ex + '';
}
TEST(1, expect, actual);

END();
