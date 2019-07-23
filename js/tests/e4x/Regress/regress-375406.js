





































gTestfile = 'regress-375406.js';

var summary = 'Do not crash @ PutProperty setting <a/>.attribute("")[0]';
var BUGNUMBER = 375406;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

<a/>.attribute('')[0] = 1;

TEST(1, expect, actual);

END();
