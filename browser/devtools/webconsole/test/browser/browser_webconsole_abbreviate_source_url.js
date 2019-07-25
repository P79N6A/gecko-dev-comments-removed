





function test() {
  testAbbreviation("http://example.com/x.js", "x.js");
  testAbbreviation("http://example.com/foo/bar/baz/boo.js", "boo.js");
  testAbbreviation("http://example.com/foo/bar/", "bar");
  testAbbreviation("http://example.com/foo.js?bar=1&baz=2", "foo.js");
  testAbbreviation("http://example.com/foo/?bar=1&baz=2", "foo");

  finishTest();
}

function testAbbreviation(aFullURL, aAbbreviatedURL) {
  is(ConsoleUtils.abbreviateSourceURL(aFullURL), aAbbreviatedURL, aFullURL +
     " is abbreviated to " + aAbbreviatedURL);
}

