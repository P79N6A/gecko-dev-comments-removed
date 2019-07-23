








































gTestfile = 'regress-278112.js';

START('setNamespace() should not affect namespaceDeclarations()');
printBugNumber('278112');

var xhtml1NS = new Namespace('http://www.w3.org/1999/xhtml');
var xhtml = <html />;
xhtml.setNamespace(xhtml1NS);

TEST(1, 0, xhtml.namespaceDeclarations().length);

TEST(2, xhtml1NS, xhtml.namespace());

END();
