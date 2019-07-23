




































var count = 0;

function trace(msg)
{
  dump("test " + msg + " (" + count + ")\n");
}

function dump2(msg, parent, child)
{
  dump(msg);
  dump(" parent=");
  dump(parent.tagName);
  dump(" child=");
  dump((child.nodeType != Node.TEXT_NODE)  ? child.tagName : "(text)");

  dump(" kids: ");
  var children = parent.childNodes;
  var length = children.length;
  var child = null;
  var count = 0;
  while (count < length) {
    child = children.item(count);
    dump((child.nodeType != Node.TEXT_NODE)  ? child.tagName : "(text)");
    dump(",");
    count++;
  }

  dump("\n");
}


function firstChildOf(node)
{
  return node.childNodes.item(0);
}

function middleChildOf(node)
{
  var children = node.childNodes;
  return children.item(Math.floor(children.length / 2));
}

function lastChildOf(node)
{
  var children = node.childNodes;
  return children.item(children.length - 1)
}

function findContainer(node, name)
{
  dump("Looking in " + node.tagName + " for " + name + "\n");
  var children = node.childNodes;
  var length = children.length;
  var child = null;
  var count = 0;
  while (count < length) {
    child = children.item(count);
    if (child.nodeType != Node.TEXT_NODE) {
      if (child.tagName == name) {
        return child;
      }
      var body = findContainer(child, name);
      if (null != body) {
        return body;
      }
    }
    count++;
  }
  return null;
}

var longText = "Lots more text (this text is long so that we can verify that continuations are handled properly when they are deleted) ";

function makeTestDocument(node)
{
  var blockTag = new Array(6);
  blockTag[0] = "DIV";
  blockTag[1] = "ADDRESS";
  blockTag[2] = "BLOCKQUOTE";
  blockTag[3] = "CENTER";
  blockTag[4] = "H1";
  blockTag[5] = "H2";

  var spanTag = new Array(6);
  spanTag[0] = "SPAN";
  spanTag[1] = "B";
  spanTag[2] = "I";
  spanTag[3] = "EM";
  spanTag[4] = "STRONG";
  spanTag[5] = "TT";

  for (b = 0; b < 6; b++) {
    var block = document.createElement(blockTag[b]);
    block.insertBefore(document.createTextNode("Opening text ["), null);
    for (s = 0; s < 6; s++) {
      var span = document.createElement(spanTag[s]);
      block.insertBefore(span, null);
      span.insertBefore(document.createTextNode("More text, "), null);
      span.insertBefore(document.createTextNode("And more text, "), null);
      span.insertBefore(document.createTextNode("And even more text. "), null);
      span.insertBefore(document.createTextNode(longText), null);
      span.insertBefore(document.createTextNode("Because more text "), null);
      span.insertBefore(document.createTextNode("Is more text."), null);
    }
    block.insertBefore(document.createTextNode("] Closing text"), null);

    
    node.insertBefore(block, null);
  }
}


function testDelete(node)
{
  var kid = firstChildOf(node);
  dump2("Remove first child", node, kid);
  node.removeChild(kid);

  kid = lastChildOf(node);
  dump2("Remove last child", node, kid);
  node.removeChild(kid);

  kid = middleChildOf(node);
  dump2("Remove middle child", node, kid);
  node.removeChild(kid);
}

var body = findContainer(document.documentElement, "BODY");
makeTestDocument(body);




var block = findContainer(body, "DIV");



testDelete(body);
