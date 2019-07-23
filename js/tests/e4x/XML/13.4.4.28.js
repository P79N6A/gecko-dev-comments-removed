









































gTestfile = '13.4.4.28.js';

START("13.4.4.28 - processingInsructions()");

TEST(1, true, XML.prototype.hasOwnProperty("processingInstructions"));

XML.ignoreProcessingInstructions = false;


x = <alpha><?xyz abc="123" michael="wierd"?><?another name="value" ?><bravo>one</bravo></alpha>;

correct = <><?xyz abc="123" michael="wierd"?><?another name="value" ?></>;

TEST(2, correct, x.processingInstructions());
TEST(3, correct, x.processingInstructions("*"));

correct = "<?xyz abc=\"123\" michael=\"wierd\"?>";

TEST_XML(4, correct, x.processingInstructions("xyz"));











END();
