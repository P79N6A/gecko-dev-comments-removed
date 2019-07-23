





































gTestfile = 'regress-370048-02.js';

var BUGNUMBER = 370048;
var summary = 'with (obj) function:: with xml lists';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var list = <><a/></>;
expect = list.function::addNamespace;
with (list)
    actual = function::addNamespace;

if (actual !== expect)
    throw "Inconsistent xml list view through property access and with statement."
TEST(1, expect, actual);
END();
