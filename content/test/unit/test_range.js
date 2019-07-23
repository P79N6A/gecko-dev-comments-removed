



































const C_i = Components.interfaces;
const C_r = Components.results;

const UNORDERED_TYPE = C_i.nsIDOMXPathResult.ANY_UNORDERED_NODE_TYPE;

const INVALID_STATE_ERR = 0x8053000b;     
const INDEX_SIZE_ERR = 0x80530001;        
const INVALID_NODE_TYPE_ERR = 0x805c0002; 
const NOT_OBJECT_ERR = 0x805303eb;        
const SECURITY_ERR = 0x805303e8;          







function isWhitespace(aNode) {
  return ((/\S/).test(aNode.nodeValue)) ?
         C_i.nsIDOMNodeFilter.FILTER_SKIP :
         C_i.nsIDOMNodeFilter.FILTER_ACCEPT;
}








function getFragment(aNode) {
  var frag = aNode.ownerDocument.createDocumentFragment();
  do_check_true(frag instanceof C_i.nsIDOM3Node);
  for (var i = 0; i < aNode.childNodes.length; i++) {
    frag.appendChild(aNode.childNodes.item(i).cloneNode(true));
  }
  return frag;
}


const serializer = new DOMSerializer();
const parser = new DOMParser();






function dumpFragment(aFragment) {
  dump(serializer.serializeToString(aFragment) + "\n\n");
}










function evalXPathInDocumentFragment(aContextNode, aPath) {
  do_check_true(aContextNode instanceof C_i.nsIDOMDocumentFragment);
  do_check_true(aContextNode.childNodes.length > 0);
  if (aPath == ".") {
    return aContextNode;
  }

  
  var firstSlash = aPath.indexOf("/");
  if (firstSlash == -1) {
    firstSlash = aPath.length;
  }
  var prefix = aPath.substr(0, firstSlash);
  var realPath = aPath.substr(firstSlash + 1);
  if (!realPath) {
    realPath = ".";
  }

  
  var childIndex = 1;
  var bracketIndex = prefix.indexOf("[");
  if (bracketIndex != -1) {
    childIndex = Number(prefix.substring(bracketIndex + 1, prefix.indexOf("]")));
    do_check_true(childIndex > 0);
    prefix = prefix.substr(0, bracketIndex);
  }

  var targetType = C_i.nsIDOMNodeFilter.SHOW_ELEMENT;
  var targetNodeName = prefix;
  if (prefix.indexOf("processing-instruction(") == 0) {
    targetType = C_i.nsIDOMNodeFilter.SHOW_PROCESSING_INSTRUCTION;
    targetNodeName = prefix.substring(prefix.indexOf("(") + 2, prefix.indexOf(")") - 1);
  }
  switch (prefix) {
    case "text()":
      targetType = C_i.nsIDOMNodeFilter.SHOW_TEXT |
                   C_i.nsIDOMNodeFilter.SHOW_CDATA_SECTION;
      targetNodeName = null;
      break;
    case "comment()":
      targetType = C_i.nsIDOMNodeFilter.SHOW_COMMENT;
      targetNodeName = null;
      break;
    case "node()":
      targetType = C_i.nsIDOMNodeFilter.SHOW_ALL;
      targetNodeName = null;
  }

  var filter = {
    count: 0,

    
    acceptNode: function acceptNode(aNode) {
      if (aNode.parentNode != aContextNode) {
        
        return C_i.nsIDOMNodeFilter.FILTER_REJECT;
      }

      if (targetNodeName && targetNodeName != aNode.nodeName) {
        return C_i.nsIDOMNodeFilter.FILTER_SKIP;
      }

      this.count++;
      if (this.count != childIndex) {
        return C_i.nsIDOMNodeFilter.FILTER_SKIP;
      }

      return C_i.nsIDOMNodeFilter.FILTER_ACCEPT;
    }
  };

  
  var walker = aContextNode.ownerDocument.createTreeWalker(
                 aContextNode,
                 targetType,
                 filter,
                 true);
  var targetNode = walker.nextNode();
  do_check_neq(targetNode, null);

  
  var expr = aContextNode.ownerDocument.createExpression(realPath, null);
  var result = expr.evaluate(targetNode, UNORDERED_TYPE, null);
  do_check_true(result instanceof C_i.nsIDOMXPathResult);
  return result.singleNodeValue;
}









function getRange(aSourceNode, aFragment) {
  do_check_true(aSourceNode instanceof C_i.nsIDOMElement);
  do_check_true(aFragment instanceof C_i.nsIDOMDocumentFragment);
  var doc = aSourceNode.ownerDocument;

  var containerPath = aSourceNode.getAttribute("startContainer");
  var startContainer = evalXPathInDocumentFragment(aFragment, containerPath);
  var startOffset = Number(aSourceNode.getAttribute("startOffset"));

  containerPath = aSourceNode.getAttribute("endContainer");
  var endContainer = evalXPathInDocumentFragment(aFragment, containerPath);
  var endOffset = Number(aSourceNode.getAttribute("endOffset"));

  var range = doc.createRange();
  range.setStart(startContainer, startOffset);
  range.setEnd(endContainer, endOffset);
  return range;
}






function getParsedDocument(aPath) {
  var doc = do_parse_document(aPath, "application/xml");
  do_check_true(doc.documentElement.localName != "parsererror");
  do_check_true(doc instanceof C_i.nsIDOMDocumentTraversal);
  do_check_true(doc instanceof C_i.nsIDOMXPathEvaluator);
  do_check_true(doc instanceof C_i.nsIDOMDocumentRange);

  
  var walker = doc.createTreeWalker(doc,
                                    C_i.nsIDOMNodeFilter.SHOW_TEXT |
                                    C_i.nsIDOMNodeFilter.SHOW_CDATA_SECTION,
                                    isWhitespace,
                                    false);
  while (walker.nextNode()) {
    var parent = walker.currentNode.parentNode;
    parent.removeChild(walker.currentNode);
    walker.currentNode = parent;
  }

  
  var splits = doc.getElementsByTagName("split");
  var i;
  for (i = splits.length - 1; i >= 0; i--) {
    var node = splits.item(i);
    node.parentNode.removeChild(node);
  }
  splits = null;

  
  var emptyData = doc.getElementsByTagName("empty-cdata");
  for (i = emptyData.length - 1; i >= 0; i--) {
    var node = emptyData.item(i);
    var cdata = doc.createCDATASection("");
    node.parentNode.replaceChild(cdata, node);
  }

  return doc;
}




function run_extract_test() {
  var filePath = "test_delete_range.xml";
  var doc = getParsedDocument(filePath);
  var tests = doc.getElementsByTagName("test");

  
  for (var i = 0; i < tests.length; i++) {
    dump("Configuring for test " + i + "\n");
    var currentTest = tests.item(i);

    
    var baseSource = currentTest.firstChild;
    do_check_eq(baseSource.nodeName, "source");
    var baseResult = baseSource.nextSibling;
    do_check_eq(baseResult.nodeName, "result");
    var baseExtract = baseResult.nextSibling;
    do_check_eq(baseExtract.nodeName, "extract");
    do_check_eq(baseExtract.nextSibling, null);

    




















    var resultFrag = getFragment(baseResult);
    var extractFrag = getFragment(baseExtract);

    dump("Extract contents test " + i + "\n\n");
    var baseFrag = getFragment(baseSource);
    var baseRange = getRange(baseSource, baseFrag);
    var startContainer = baseRange.startContainer;
    var endContainer = baseRange.endContainer;

    var cutFragment = baseRange.extractContents();
    dump("cutFragment: " + cutFragment + "\n");
    if (cutFragment) {
      do_check_true(extractFrag.isEqualNode(cutFragment));
    } else {
      do_check_eq(extractFrag.firstChild, null);
    }
    do_check_true(baseFrag.isEqualNode(resultFrag));

    dump("Ensure the original nodes weren't extracted - test " + i + "\n\n");
    var walker = doc.createTreeWalker(baseFrag,
				      C_i.nsIDOMNodeFilter.SHOW_ALL,
				      null,
				      false);
    var foundStart = false;
    var foundEnd = false;
    do {
      do_check_true(walker.currentNode instanceof C_i.nsIDOM3Node);

      if (walker.currentNode.isSameNode(startContainer)) {
        foundStart = true;
      }

      if (walker.currentNode.isSameNode(endContainer)) {
        
        do_check_true(foundStart);
        foundEnd = true;
        break;
      }
    } while (walker.nextNode())
    do_check_true(foundEnd);

    




    dump("Delete contents test " + i + "\n\n");
    baseFrag = getFragment(baseSource);
    baseRange = getRange(baseSource, baseFrag);
    var startContainer = baseRange.startContainer;
    var endContainer = baseRange.endContainer;
    baseRange.deleteContents();
    do_check_true(baseFrag.isEqualNode(resultFrag));

    dump("Ensure the original nodes weren't deleted - test " + i + "\n\n");
    walker = doc.createTreeWalker(baseFrag,
                                  C_i.nsIDOMNodeFilter.SHOW_ALL,
                                  null,
                                  false);
    foundStart = false;
    foundEnd = false;
    do {
      do_check_true(walker.currentNode instanceof C_i.nsIDOM3Node);

      if (walker.currentNode.isSameNode(startContainer)) {
        foundStart = true;
      }

      if (walker.currentNode.isSameNode(endContainer)) {
        
        do_check_true(foundStart);
        foundEnd = true;
        break;
      }
    } while (walker.nextNode())
    do_check_true(foundEnd);

    
    walker = null;

    



    dump("Detached range test\n");
    var compareFrag = getFragment(baseSource);
    baseFrag = getFragment(baseSource);
    baseRange = getRange(baseSource, baseFrag);
    baseRange.detach();
    try {
      var cutFragment = baseRange.extractContents();
      do_throw("Should have thrown INVALID_STATE_ERR!");
    } catch (e if (e instanceof C_i.nsIException &&
                   e.result == INVALID_STATE_ERR)) {
      
    }
    do_check_true(compareFrag.isEqualNode(baseFrag));

    try {
      baseRange.deleteContents();
      do_throw("Should have thrown INVALID_STATE_ERR!");
    } catch (e if (e instanceof C_i.nsIException &&
                   e.result == INVALID_STATE_ERR)) {
      
    }
    do_check_true(compareFrag.isEqualNode(baseFrag));
  }
}




function run_miscellaneous_tests() {
  var filePath = "test_delete_range.xml";
  var doc = getParsedDocument(filePath);
  var tests = doc.getElementsByTagName("test");

  
  var currentTest = tests.item(0);
  var baseSource = currentTest.firstChild;
  var baseResult = baseSource.nextSibling;
  var baseExtract = baseResult.nextSibling;

  var baseFrag = getFragment(baseSource);

  var baseRange = getRange(baseSource, baseFrag);
  var startContainer = baseRange.startContainer;
  var endContainer = baseRange.endContainer;
  var startOffset = baseRange.startOffset;
  var endOffset = baseRange.endOffset;

  
  if ((endOffset > startOffset) &&
      (startContainer == endContainer) &&
      (startContainer instanceof C_i.nsIDOMText)) {
    
    try {
      baseRange.setStart(null, 0);
      do_throw("Should have thrown NOT_OBJECT_ERR!");
    } catch (e if (e instanceof C_i.nsIException &&
                   e.result == NOT_OBJECT_ERR)) {
      
    }

    
    try {
      baseRange.setStart({}, 0);
      do_throw("Should have thrown SECURITY_ERR!");
    } catch (e if (e instanceof C_i.nsIException &&
                   e.result == SECURITY_ERR)) {
      
    }

    
    try {
      baseRange.setStart(startContainer, -1);
      do_throw("Should have thrown INVALID_STATE_ERR!");
    } catch (e if (e instanceof C_i.nsIException &&
                   e.result == INDEX_SIZE_ERR)) {
      
    }
  
    
    var newOffset = startContainer instanceof C_i.nsIDOMText ?
                      startContainer.nodeValue.length + 1 :
                      startContainer.childNodes.length + 1;
    try {
      baseRange.setStart(startContainer, newOffset);
      do_throw("Should have thrown INVALID_STATE_ERR!");
    } catch (e if (e instanceof C_i.nsIException &&
                   e.result == INDEX_SIZE_ERR)) {
      
    }
  
    newOffset--;
    
    baseRange.setStart(startContainer, newOffset);
    do_check_eq(baseRange.startContainer, baseRange.endContainer);
    do_check_eq(baseRange.startOffset, newOffset);
    do_check_true(baseRange.collapsed);

    
    baseRange.setEnd(startContainer, 0);
    do_check_eq(baseRange.startContainer, baseRange.endContainer);
    do_check_eq(baseRange.startOffset, 0);
    do_check_true(baseRange.collapsed);
  } else {
    do_throw("The first test should be a text-only range test.  Test is invalid.")
  }

  


  baseRange = getRange(baseSource, baseFrag);
  startContainer = baseRange.startContainer;
  var startOffset = baseRange.startOffset;
  endContainer = baseRange.endContainer;
  var endOffset = baseRange.endOffset;

  dump("External fragment test\n\n");

  var externalTest = tests.item(1);
  var externalSource = externalTest.firstChild;
  var externalFrag = getFragment(externalSource);
  var externalRange = getRange(externalSource, externalFrag);

  baseRange.setEnd(externalRange.endContainer, 0);
  do_check_eq(baseRange.startContainer, externalRange.endContainer);
  do_check_eq(baseRange.startOffset, 0);
  do_check_true(baseRange.collapsed);

  







  
  doc = parser.parseFromString("<!-- foo --><foo/>", "application/xml");
  do_check_true(doc instanceof C_i.nsIDOMDocumentRange);
  do_check_eq(doc.childNodes.length, 2);
  baseRange = doc.createRange();
  baseRange.setStart(doc.firstChild, 1);
  baseRange.setEnd(doc.firstChild, 2);
  var frag = baseRange.extractContents();
  do_check_eq(frag.childNodes.length, 1);
  do_check_true(frag.firstChild instanceof C_i.nsIDOMComment);
  do_check_eq(frag.firstChild.nodeValue, "f");

  


}

function run_test() {
  run_extract_test();
  run_miscellaneous_tests();
}
