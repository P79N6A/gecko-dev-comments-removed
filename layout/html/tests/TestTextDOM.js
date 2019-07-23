




































function testInsert(text)
{
  text.deleteData(0, 99999);

  
  text.insertData(0, "word");
  if (text.data != "word") {
    dump("testInsert Error(0): '" + text.data + "' != word\n");
  }

  
  text.insertData(1, "Z");
  if (text.data != "wZord") {
    dump("testInsert Error(1): '" + text.data + "' != wZord\n");
  }
  text.insertData(4, "Z");
  if (text.data != "wZorZd") {
    dump("testInsert Error(2): '" + text.data + "' != wZorZd\n");
  }

  
  text.insertData(99, "Z");
  if (text.data != "wZorZdZ") {
    dump("testInsert Error(3): '" + text.data + "' != wZorZdZ\n");
  }

  
  text.insertData(-9000, "first");
  if (text.data != "firstwZorZdZ") {
    dump("testInsert Error(4): '" + text.data + "' != firstwZorZdZ\n");
  }
}

function testAppend(text)
{
  text.deleteData(0, 99999);

  
  text.appendData("word");
  if (text.data != "word") {
    dump("testAppend Error(0): '" + text.data + "' != word\n");
  }

  
  text.appendData("word");
  if (text.data != "wordword") {
    dump("testAppend Error(1): '" + text.data + "' != wordword\n");
  }
}

function testDelete(text)
{
  text.deleteData(0, 99999);
  text.appendData("wordwo3rd");

  
  text.deleteData(0, 4);
  if (text.data != "wo3rd") {
    dump("testDelete Error(0): '" + text.data + "' != wo3rd\n");
  }

  
  text.deleteData(2, 1);
  if (text.data != "word") {
    dump("testDelete Error(1): '" + text.data + "' != word\n");
  }

  
  text.deleteData(2, 2);
  if (text.data != "wo") {
    dump("testDelete Error(2): '" + text.data + "' != wo\n");
  }

  
  text.deleteData(0, 2);
  if (text.data != "") {
    dump("testDelete Error(3): '" + text.data + "' != ''\n");
  }

  
  text.appendData("word");
  text.deleteData(-100, -10);
  if (text.data != "word") {
    dump("testDelete Error(4): '" + text.data + "' != word\n");
  }
  text.deleteData(-100, 0);
  if (text.data != "word") {
    dump("testDelete Error(5): '" + text.data + "' != word\n");
  }
  text.deleteData(0, -10);
  if (text.data != "word") {
    dump("testDelete Error(6): '" + text.data + "' != word\n");
  }
  text.deleteData(0, 0);
  if (text.data != "word") {
    dump("testDelete Error(7): '" + text.data + "' != word\n");
  }
}

function testReplace(text)
{
  text.deleteData(0, 99999);
  text.appendData("word");

  
  text.replaceData(0, 0, "fish");
  if (text.data != "fishword") {
    dump("testReplace Error(0): '" + text.data + "' != fisword\n");
  }

  
  text.replaceData(0, 4, "");
  if (text.data != "word") {
    dump("testReplace Error(1): '" + text.data + "' != word\n");
  }

  
  text.replaceData(1, 1, "fish");
  if (text.data != "wfishrd") {
    dump("testReplace Error(2): '" + text.data + "' != wfishrd\n");
  }

  
  text.replaceData(1, 4, "");
  if (text.data != "wrd") {
    dump("testReplace Error(3): '" + text.data + "' != wrd\n");
  }

  
  text.replaceData(0, 1, "W");
  text.replaceData(1, 1, "O");
  text.replaceData(2, 1, "R");
  text.replaceData(3, 1, "D");
  if (text.data != "WORD") {
    dump("testReplace Error(4): '" + text.data + "' != WORD\n");
  }

  
  text.replaceData(4, 97, "FISH");
  if (text.data != "WORDFISH") {
    dump("testReplace Error(5): '" + text.data + "' != WORDFISH\n");
  }

  
  text.replaceData(-99, 4, "SWORD");
  if (text.data != "SWORDFISH") {
    dump("testReplace Error(6): '" + text.data + "' != SWORDFISH\n");
  }
}

function testText(text)
{
  testInsert(text);
  testAppend(text);
  testDelete(text);
  testReplace(text);
}

function findText(container)
{
  if (container.hasChildNodes()) {
    
    
    var children = container.childNodes;
    var length = children.length;
    var child = null;
    var count = 0;
    while (count < length) {
      child = children[count];
      if (child.nodeType == Node.TEXT_NODE) {
        return child;
      }
      var text = findText(child);
      if (null != text) {
        return text;
      }
      count++;
    }
  }
  return null;
}

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

var body = findBody(document.documentElement)
var text = findText(body)
testText(text)
dump("Test finished\n");
