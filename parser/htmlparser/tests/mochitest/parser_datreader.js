















































function log(entry) {
    
}

function startsWith(s, s2) {
  return s.indexOf(s2)==0;
}

function trimString(s) {
  return(s.replace(/^\s+/,'').replace(/\s+$/,''));
}

function getLastLine(str) {
  var str_array = str.split("\n");
  let last_line = str_array[str_array.length - 1];
  return last_line;
}










var gDumpMode;
function dumpTree(buf, obj, indent) {
  var buffer = buf;
  if (typeof(obj) == "object" && (obj instanceof Array)) {
    for each (var item in obj) {
      [buffer, indent] = dumpTree(buffer, item, indent);
    }
    gDumpMode = -1;
  }
  else {
    
    switch(obj) {
      case "ParseError":
        
        break;
      case "Character":
        gDumpMode = Node.TEXT_NODE;
        break;
      case "StartTag":
        gDumpMode = Node.ELEMENT_NODE;
        break;
      case "EndTag":
        indent = indent.substring(2);
        break;
      case "Comment":
        gDumpMode = Node.COMMENT_NODE;
        break;
      case "DOCTYPE":
        gDumpMode = Node.DOCUMENT_TYPE_NODE;
        break;
      default:
        switch(gDumpMode) {
          case Node.DOCUMENT_TYPE_NODE:
            buffer += "<!DOCTYPE " + obj + ">\n<html>\n  <head>\n  <body>";
            indent += "    "
            gDumpMode = -1;
            break;
          case Node.COMMENT_NODE:
            if (buffer.length > 1) {
              buffer += "\n";
            }
            buffer += indent + "<!-- " + obj + " -->";
            gDumpMode = -1;
            break;
          case Node.ATTRIBUTE_NODE:
            is(typeof(obj), "object", "obj not an object!");
            indent += "  ";
            for (var key in obj) {
              buffer += "\n" + indent + key + "=\"" + obj[key] + "\"";
            }
            gDumpMode = -1;
            break;
          case Node.TEXT_NODE:
            if (buffer.indexOf("<head>") == -1) {
              buffer += "\n<html>\n  <head>\n  <body>";
              indent += "    ";
            }
            
            
            
            let last_line = trimString(getLastLine(buffer));
            if (last_line[0] == "\"" && 
              last_line[last_line.length - 1] == "\"") {
              buffer = buffer.substring(0, buffer.length - 1);    
            }
            else {
              buffer += "\n" + indent + "\"";  
            }
            buffer += obj + "\"";
            break;
          case Node.ELEMENT_NODE:
            buffer += "\n" + indent + "<" + obj + ">";
            gDumpMode = Node.ATTRIBUTE_NODE;
            break;
          default:
            
            break;
        }
        break;
    }
  }
  return [buffer, indent];
}







function parseJsonTestcase(testcase) {
  gDumpMode = -1;
  
  
  
  if (testcase["input"].toLowerCase().indexOf("<!doc") == 0) {
    var test_output = dumpTree(
      "", 
      testcase["output"],
      "");
  } else {
    var test_output = dumpTree(
      "<!DOCTYPE html>\n<html>\n  <head>\n  <body>", 
      testcase["output"],
      "    ");
  }
  
  
  if (test_output[0].indexOf("<head>") == -1) {
    test_output[0] += "\n<html>\n  <head>\n  <body>";
  }
  return [testcase["input"], test_output[0], "",
    testcase["description"],
    JSON.stringify(testcase["output"])];
}








function parseTestcase(testcase) {
  var documentFragmentTest = false;
  var lines = testcase.split("\n");
  if (lines[0] != "#data") {
    log(lines);
    throw "Unknown test format."
  }
  var input = [];
  var output = [];
  var errors = [];
  var description = undefined;
  var expectedTokenizerOutput = undefined;
  var currentList = input;
  for each (var line in lines) {
    
    if ((line || currentList == input) && !(startsWith(line, "#errors") ||
		  startsWith(line, "#document") ||
		  startsWith(line, "#description") ||
		  startsWith(line, "#expected") || 
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
    } else if (line == "#document-fragment") {
      documentFragmentTest = true;
    }
  }  
  
  
  
  
  if (documentFragmentTest) {
    output = [];
  }
  
  return [input.join("\n"), output.join("\n"), errors, description,
    expectedTokenizerOutput];
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

  for (var i=0; i < outputLines.length; i++) {
    var outputLine = outputLines[i];
    var expectedLine = expectedLines[i];
    var inAttrList = false;
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
  var index = 1;
  if (gTokenizerMode) {
    for each (var test in testlist) {
      var tmpArray = [index];
      yield tmpArray.concat(parseJsonTestcase(test));
      index++;
    }
  }
  else {
    for each (var testgroup in testlist) {
      var tests = testgroup.split("#data\n");
      tests = ["#data\n" + test for each(test in tests) if (test)];
      for each (var test in tests) {
        yield parseTestcase(test);
      }
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
      switch (walker.currentNode.nodeType) {
        case Node.ELEMENT_NODE:
          buf += indent + "<";
          
          
          if (walker.currentNode.namespaceURI.toLowerCase().
          indexOf("math") != -1) {
            buf += "math " + walker.currentNode.tagName.toLowerCase() + ">\n";
          }
          else if (walker.currentNode.namespaceURI.toLowerCase().
          indexOf("svg") != -1) {
            buf += "svg " + walker.currentNode.tagName + ">\n";
          }
          else {
            buf += walker.currentNode.tagName.toLowerCase() + ">\n";
          }
          if (walker.currentNode.hasAttributes()) {
            var attrs = walker.currentNode.attributes;
            for (var i=0; i < attrs.length; ++i) {
              
              
              var attrname = attrs[i].name;
              if (attrname != "-moz-math-font-style") {
                buf += indent + "  " + attrname + 
                       "=\"" + attrs[i].value +"\"\n";
              }
            }
          }
          break;
        case Node.DOCUMENT_TYPE_NODE:
          if (!gJSCompatibilityMode) {
            buf += indent + "<!DOCTYPE " + walker.currentNode.name + ">\n";
          }
          break;
        case Node.COMMENT_NODE:
          if (!gJSCompatibilityMode) {
            buf += indent + "<!-- " + walker.currentNode.nodeValue + " -->\n";
          }
          break;
        case Node.TEXT_NODE:
          
          
          
          let last_line = getLastLine(
            buf.substring(0, buf.length - 1));
          if (last_line[indent.length] == "\"" && 
            last_line[last_line.length - 1] == "\"") {
            buf = buf.substring(0, buf.length - 2);    
          }
          else {
            buf += indent + "\"";
          }
          buf += walker.currentNode.nodeValue + "\"\n";
          break;
      }
      buf = addLevels(walker, buf, indent + "  ");
    } while(walker.nextSibling());
    walker.parentNode();
  }
  return buf;
}

