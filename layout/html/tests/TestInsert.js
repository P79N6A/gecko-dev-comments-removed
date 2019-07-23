




































var count = 0;

function trace(msg)
{
  dump("test " + msg + " (" + count + ")\n");
}

function findBody(node)
{
  var children = node.childNodes;
  var length = children.length;
  var child = null;
  var count = 0;
  while (count < length) {
    child = children.item(count);
    if (child.tagName == "BODY") {
      return child;
    }
    var body = findBody(child);
    if (null != body) {
      return body;
    }
    count++;
  }
  return null;
}

function TestInsert(parent, child)
{
  var childTag = "(text)";
  if (child.nodeType != Node.TEXT_NODE) {
    childTag = child.tagName;
  }

  
  
  trace("insert before [" + parent.tagName + "," + childTag + "]");
  count++;
  var beforeText = document.createTextNode("before ");
  parent.insertBefore(beforeText, child);

  
  
  trace("insert middle [" + parent.tagName + "," + childTag + "]");
  count++;
  parent.insertBefore(document.createTextNode("middle "), child);
}

var body = findBody(document.documentElement);


var block = document.createElement("P");
var span = document.createElement("SPAN");
var spanText = document.createTextNode("Some text");
span.insertBefore(spanText, null);
block.insertBefore(span, null);


body.insertBefore(block, null);


TestInsert(block, span);


TestInsert(span, spanText);
