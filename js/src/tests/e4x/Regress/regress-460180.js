





































gTestfile = 'regress-460180.js';

var summary = 'Do not crash with if (false || false || <x/>) {}';
var BUGNUMBER = 460180;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

if (false || false || <x/>) { }

TEST(1, expect, actual);

END();
