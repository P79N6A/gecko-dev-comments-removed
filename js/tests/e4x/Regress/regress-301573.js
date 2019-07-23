





































gTestfile = 'regress-301573.js';

var summary = "E4X - Entities";
var BUGNUMBER = 301573;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

actual = <xml>&lt;</xml>;

TEST(1, '<', actual.toString());

actual = <xml>&amp;</xml>;

TEST(2, '&', actual.toString());

END();
