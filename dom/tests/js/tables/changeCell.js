




































function findBody(node)
{
  if (node.nodeType != Node.ELEMENT_NODE) {
    return null;
  }
  var children = node.childNodes;
  if (children == null) {
    return null;
  }
  var length = children.length;
  var child = null;
  var count = 0;
  while (count < length) {
    child = children[count];
    if (child.tagName == "BODY") {
      dump("BODY found");
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


function findTable(body)
{
  
  
  var children = body.childNodes
  if (children == null) {
    return null;
  }
  var length = children.length;
  var child = null;
  var count = 0;
  while (count < length) {
    child = children[count];
    if (child.nodeType == Node.ELEMENT_NODE) {
      if (child.tagName == "TABLE") {
        dump("TABLE found");
        break;
      }
    }
    count++;
  }

  return child;
}


function findTableBody(table)
{
  
  
  var children = table.childNodes
  if (children == null) {
    return null;
  }
  var length = children.length;
  var child = null;
  var count = 0;
  while (count < length) {
    child = children[count];
    if (child.nodeType == Node.ELEMENT_NODE) {
      if (child.tagName == "TBODY") {
        break;
      }
    }
    count++;
  }

  return child;
}


function changeCell(table)
{
  
  var body = findTableBody(table)

  
  var row = body.firstChild

  
  var cell = row.firstChild

  
  var text = cell.firstChild

  
  text.append(" NEW TEXT")
}

changeCell(findTable(findBody(document.documentElement)))

