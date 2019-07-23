

function traverse(node, indent)
{
  indent += "  ";
  var type = node.nodeType;

  
  if (type == Node.ELEMENT_NODE) {
    dump(indent + "<" + node.tagName + ">\n");

    
    if (node.hasChildNodes()) {
      var children = node.childNodes;
      var i, length = children.length;
      for (i = 0; i < length; i++) {
        var child = children[i];
        traverse(child, indent);
      }
      dump(indent + "</" + node.tagName + ">\n");
    }
  }
  
  else if (type == Node.TEXT_NODE) {
    dump(indent + node.data + "\n");
  }
}

function dumpTree()
{
  var node = document.documentElement;
  dump("Document Tree:\n");
  traverse(node, "");
  dump("\n");
}
