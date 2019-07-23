





































gTestfile = 'regress-474319.js';

var summary = 'Do not crash with e4x, map and concat';
var BUGNUMBER = 474319;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

if (typeof gczeal != 'function' || !('map' in Array.prototype))
{
    expect = actual = 'Test skipped due to lack of gczeal and Array.prototype.map';
}
else
{
    gczeal(2);
    (function(){[<y><z/></y>].map(''.concat)})();
}
TEST(1, expect, actual);

END();
