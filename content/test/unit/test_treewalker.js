




function run_test()
{
  test_treeWalker_currentNode();
}



function test_treeWalker_currentNode()
{
  var XHTMLDocString = '<html xmlns="http://www.w3.org/1999/xhtml">';
  XHTMLDocString += '<body><input/>input</body></html>';

  var doc = ParseXML(XHTMLDocString);

  var body = doc.getElementsByTagName("body")[0];
  var filter = I.nsIDOMNodeFilter.SHOW_ELEMENT | I.nsIDOMNodeFilter.SHOW_TEXT;
  var walker = doc.createTreeWalker(body, filter, null);
  walker.currentNode = body.firstChild;
  walker.nextNode();
}

