





































var bug = 373072;
var summary = 'XML.prototype.namespace() does not check for xml list';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

XML.prototype.function::namespace.call(new XMLList());
TEST(1, expect, actual);

END();
