







































START("If document starts with comment, document is discarded");
BUG(291930);

XML.ignoreComments = false;
try {
	var root = new XML("<!-- Sample --> <root/>");
	SHOULD_THROW(1, "SyntaxError");
} catch (e) {
	TEST(1, "error", "error");
}

END();