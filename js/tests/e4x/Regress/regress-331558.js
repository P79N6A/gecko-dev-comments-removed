





































gTestfile = 'regress-331558.js';

var BUGNUMBER = 331558;
var summary = 'Decompiler: Missing = in default xml namespace statement';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

actual = (function () { default xml namespace = 'abc' }).toString();
expect = 'function () {\n    default xml namespace = "abc";\n}';

TEST(1, expect, actual);

END();
