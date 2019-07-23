




































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
    child = children[count];
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

function AppendTest(parent, kidTag, grandKidTag, empty, asWeGo)
{
  trace("enter [" + kidTag + "," + (grandKidTag?grandKidTag:"") + "]");
  var kid = document.createElement(kidTag);
  if (asWeGo) {
    parent.insertBefore(kid, null);
  }
  if (null != grandKidTag) {
    var grandKid = document.createElement(grandKidTag);
    if (empty) {
      kid.insertBefore(grandKid, null);
    }
    else {
      if (asWeGo) {
        kid.insertBefore(grandKid, null);
      }
      var text = document.createTextNode("inner text");
      grandKid.insertBefore(text, null);
      if (!asWeGo) {
        kid.insertBefore(grandKid, null);
      }
    }
  }
  if (!asWeGo) {
    parent.insertBefore(kid, null);
  }
  trace("exit [" + kidTag + "," + (grandKidTag?grandKidTag:"") + "]");
  count++;
}


function RunTests(parent, asWeGo)
{
  
  AppendTest(parent, "P", null, true, asWeGo);

  
  AppendTest(parent, "I", null, true, asWeGo);

  
  AppendTest(parent, "P", "P", false, asWeGo);
  AppendTest(parent, "P", "I", false, asWeGo);

  
  AppendTest(parent, "I", "P", false, asWeGo);
  AppendTest(parent, "I", "TT", false, asWeGo);
}

var body = findBody(document.documentElement);

RunTests(body, false);
RunTests(body, true);

var inline = document.createElement("SPAN");
body.insertBefore(inline, null);
RunTests(inline, false);
RunTests(inline, true);
