














function log(entry) {
    
}

function startsWith(s, s2) {
  return s.indexOf(s2)==0;
}

function trimString(s) {
  return(s.replace(/^\s+/,'').replace(/\s+$/,''));
}








function parseTestcase(testcase) {
  var lines = testcase.split("\n");

  
  for each (var line in lines) {
    if (!line || startsWith(line, "##")) {
      continue;
    }
    if (line == "#data")
      break;
    log(lines);
    throw "Unknown test format."
  }

  var input = [];
  var output = [];
  var errors = [];
  var fragment = [];
  var currentList = input;
  for each (var line in lines) {
    if (startsWith(line, "##todo")) {
      todo(false, line.substring(6));
      continue;
    }
    if (!(startsWith(line, "#error") ||
          startsWith(line, "#document") ||
          startsWith(line, "#document-fragment") ||
          startsWith(line, "#data"))) {
      currentList.push(line);
    } else if (line == "#errors") {
      currentList = errors;
    } else if (line == "#document") {
      currentList = output;
    } else if (line == "#document-fragment") {
      currentList = fragment;
    }
  }
  while (!output[output.length - 1]) {
    output.pop(); 
  }
  
  return [input.join("\n"), output.join("\n"), errors, fragment[0]];
}








function test_parser(testlist) {
  for each (var testgroup in testlist) {
    var tests = testgroup.split("#data\n");
    tests = ["#data\n" + test for each(test in tests) if (test)];
    for each (var test in tests) {
      yield parseTestcase(test);
    }
  }
}







function docToTestOutput(doc) {
  var walker = doc.createTreeWalker(doc, NodeFilter.SHOW_ALL, null);
  return addLevels(walker, "", "| ").slice(0,-1); 
}







function fragmentToTestOutput(elt) {
  var walker = elt.ownerDocument.createTreeWalker(elt, NodeFilter.SHOW_ALL, 
    function (node) { return elt == node ? 
                        NodeFilter.FILTER_SKIP : 
                        NodeFilter.FILTER_ACCEPT; });
  return addLevels(walker, "", "| ").slice(0,-1); 
}

function addLevels(walker, buf, indent) {
  if(walker.firstChild()) {
    do {
      buf += indent;
      switch (walker.currentNode.nodeType) {
        case Node.ELEMENT_NODE:
          buf += "<"
          var ns = walker.currentNode.namespaceURI;
          if ("http://www.w3.org/1998/Math/MathML" == ns) {
            buf += "math ";
          } else if ("http://www.w3.org/2000/svg" == ns) {
            buf += "svg ";
          } else if ("http://www.w3.org/1999/xhtml" != ns) {
            buf += "otherns ";
          }
          buf += walker.currentNode.localName + ">";
          if (walker.currentNode.hasAttributes()) {
            var valuesByName = {};
            var attrs = walker.currentNode.attributes;
            for (var i = 0; i < attrs.length; ++i) {
              var localName = attrs[i].localName;
              if (localName.indexOf("_moz-") == 0) {
                
                continue;
              }
              var name;
              var attrNs = attrs[i].namespaceURI;
              if (null == attrNs) {
                name = localName;
              } else if ("http://www.w3.org/XML/1998/namespace" == attrNs) {
                name = "xml " + localName;
              } else if ("http://www.w3.org/1999/xlink" == attrNs) {
                name = "xlink " + localName;
              } else if ("http://www.w3.org/2000/xmlns/" == attrNs) {
                name = "xmlns " + localName;
              } else {
                name = "otherns " + localName;
              }
              valuesByName[name] = attrs[i].value;
            }
            var keys = Object.keys(valuesByName).sort();
            for (var i = 0; i < keys.length; ++i) {
              buf += "\n" + indent + "  " + keys[i] + 
                     "=\"" + valuesByName[keys[i]] +"\"";
            }
          }
          break;
        case Node.DOCUMENT_TYPE_NODE:
          buf += "<!DOCTYPE " + walker.currentNode.name;
          if (walker.currentNode.publicId || walker.currentNode.systemId) {
            buf += " \"";
            buf += walker.currentNode.publicId;
            buf += "\" \"";
            buf += walker.currentNode.systemId;
            buf += "\"";
          }
          buf += ">";
          break;
        case Node.COMMENT_NODE:
          buf += "<!-- " + walker.currentNode.nodeValue + " -->";
          break;
        case Node.TEXT_NODE:
          buf += "\"" + walker.currentNode.nodeValue + "\"";
          break;
      }
      buf += "\n";
      buf = addLevels(walker, buf, indent + "  ");
    } while(walker.nextSibling());
    walker.parentNode();
  }
  return buf;
}

