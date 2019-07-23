





































START("E4X - Entities");

var bug = 301573;
var summary = 'parse entities';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

actual = <xml>&lt;</xml>;

TEST(1, '<', actual.toString());

actual = <xml>&amp;</xml>;

TEST(2, '&', actual.toString());

END();
