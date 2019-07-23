







































gTestfile = '13.4.4.32-01.js';

START("13.4.4.32-1 - XML replace() by index, text to string");

printBugNumber(291927);

var root = <root>text</root>;

TEST_XML(1, "text", root.child(0));

root.replace(0, "new text");

TEST_XML(2, "new text", root.child(0));
TEST(3, <root>new text</root>, root);

END();
