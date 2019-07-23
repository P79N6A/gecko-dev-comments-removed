






































gTestfile = '13.4.3.10.js';

START("13.4.3.10 - XML Constructor [[HasInstance]]");

printBugNumber(288027);

var xmlListObject1 = new XMLList('<god>Kibo</god>');
var xmlListObject2 = new XMLList('<god>Kibo</god><devil>Xibo</devil>');

TEST(1, true, xmlListObject1 instanceof XML);
TEST(2, true, xmlListObject2 instanceof XML);

END();
