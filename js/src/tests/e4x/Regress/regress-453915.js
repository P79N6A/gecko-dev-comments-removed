






































var summary = 'XML Injection possible via default xml namespace';
var BUGNUMBER = 453915;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

default xml namespace = '\'';
<foo/>

TEST(1, expect, actual);

END();
