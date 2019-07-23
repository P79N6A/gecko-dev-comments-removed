















































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
  var currentList = input;
  for each (var line in lines) {
    if (!line || startsWith(line, "##")) {
      if (startsWith(line, "##todo")) {
        todo(false, line.substring(6));
      }
      continue;
    }
    if (!(startsWith(line, "#error") ||
          startsWith(line, "#document") ||
          startsWith(line, "#data"))) {
      if (currentList == output && startsWith(line, "|")) {
      	currentList.push(line.substring(2));
      } else {
	      currentList.push(line);
      }
    } else if (line == "#errors") {
      currentList = errors;
    } else if (line == "#document") {
      currentList = output;
    }
  }  
  
  return [input.join("\n"), output.join("\n"), errors];
}








function reorderToMatchExpected(output, expected) {
  var outputLines = output.split("\n");
  var expectedLines = expected.split("\n");

  
  if (expectedLines.length != outputLines.length)
    return output;

  var fixedOutput = [];
  var outputAtts = {};
  var expectedAtts = [];
  printAtts = function() {
    for each (var expectedAtt in expectedAtts) {
      if (outputAtts.hasOwnProperty(expectedAtt)) {
        fixedOutput.push(outputAtts[expectedAtt]);
      } else {
        
        return false;
      }
    }
    outputAtts = {};
    expectedAtts = [];
    return true;
  }

  var inAttrList = false;
  for (var i=0; i < outputLines.length; i++) {
    var outputLine = outputLines[i];
    var expectedLine = expectedLines[i];
    if (isAttributeLine(outputLine)) {
      
      if (!isAttributeLine(expectedLine)) {
        return output; 
      }
      
      inAttrList = true;
      outputAtts[attName(outputLine)] = outputLine;
      expectedAtts.push(attName(expectedLine));
    } else {
      if (inAttrList && !printAtts()) {
        return output; 
      }
      inAttrList = false;
      fixedOutput.push(outputLine);
    }
  }

  if (inAttrList && !printAtts()) {
    return output; 
  }

  return fixedOutput.join("\n");
}

function attName(line) {
  var str = trimString(line);
  return str.substring(0, str.indexOf("=\""));
}

function isAttributeLine(line) {
  var str = trimString(line);
  return (!startsWith(str, "<") && !startsWith(str, "\"") &&
          (str.indexOf("=\"") > 0));
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
  var walker = doc.createTreeWalker(doc, NodeFilter.SHOW_ALL, null, true);
  return addLevels(walker, "", "").slice(0,-1); 
}

function addLevels(walker, buf, indent) {
  if(walker.firstChild()) {
    do {
      buf += indent;
      switch (walker.currentNode.nodeType) {
        case Node.ELEMENT_NODE:
          buf += "<" + walker.currentNode.tagName.toLowerCase() + ">";
          if (walker.currentNode.hasAttributes()) {
            var attrs = walker.currentNode.attributes;
            for (var i=0; i < attrs.length; ++i) {
              buf += "\n" + indent + "  " + attrs[i].name + 
                     "=\"" + attrs[i].value +"\"";
            }
          }
          break;
        case Node.DOCUMENT_TYPE_NODE:
          buf += "<!DOCTYPE " + walker.currentNode.name + ">";
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

