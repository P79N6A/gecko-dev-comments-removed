
















































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












function dumpTree(buf, obj, indent, mode) {
  var dumpMode = mode;
  if (typeof(dumpMode) == "undefined") {
    dumpMode = -1
  }
  var buffer = buf;
  if (typeof(obj) == "object" && (obj instanceof Array)) {
    for each (var item in obj) {
      [buffer, indent, dumpMode] = 
        dumpTree(buffer, item, indent, dumpMode);
    }
    dumpMode = -1;
  }
  else {
    
    switch(obj) {
      case "ParseError":
        
        break;
      case "Character":
        dumpMode = Node.TEXT_NODE;
        break;
      case "StartTag":
        dumpMode = Node.ELEMENT_NODE;
        break;
      case "EndTag":
        indent = indent.substring(2);
        break;
      case "Comment":
        dumpMode = Node.COMMENT_NODE;
        break;
      case "DOCTYPE":
        dumpMode = Node.DOCUMENT_TYPE_NODE;
        break;
      default:
        switch(dumpMode) {
          case Node.DOCUMENT_TYPE_NODE:
            buffer += "<!DOCTYPE " + obj + ">\n<html>\n  <head>\n  <body>";
            indent += "    "
            dumpMode = -1;
            break;
          case Node.COMMENT_NODE:
            if (buffer.length > 1) {
              buffer += "\n";
            }
            buffer += indent + "<!-- " + obj + " -->";
            dumpMode = -1;
            break;
          case Node.ATTRIBUTE_NODE:
            is(typeof(obj), "object", "obj not an object!");
            indent += "  ";
            for (var key in obj) {
              buffer += "\n" + indent + key + "=\"" + obj[key] + "\"";
            }
            dumpMode = -1;
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
            dumpMode = Node.ATTRIBUTE_NODE;
            break;
          default:
            
            break;
        }
        break;
    }
  }
  return [buffer, indent, dumpMode];
}







function parseJsonTestcase(testcase) {
  
  
  
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

function attName(line) {
  var str = trimString(line);
  return str.substring(0, str.indexOf("=\""));
}

function isAttributeLine(line) {
  var str = trimString(line);
  return (!startsWith(str, "<") && !startsWith(str, "\"") &&
          (str.indexOf("=\"") > 0));
}
 







function docToTestOutput(doc, mode) {
  var walker = doc.createTreeWalker(doc, NodeFilter.SHOW_ALL, null, true);
  return addLevels(walker, "", "", mode).slice(0,-1); 
}

function addLevels(walker, buf, indent, mode) {
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
          
          
          if (mode != MODE_JSCOMPARE) {
            buf += indent + "<!DOCTYPE " + walker.currentNode.name + ">\n";
          }
          break;
        case Node.COMMENT_NODE:
          
          
          if (mode != MODE_JSCOMPARE) {
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
      buf = addLevels(walker, buf, indent + "  ", mode);
    } while(walker.nextSibling());
    walker.parentNode();
  }
  return buf;
}
