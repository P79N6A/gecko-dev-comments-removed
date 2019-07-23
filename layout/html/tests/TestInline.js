



































function makeInline()
{
  var image = document.createElement("IMG");
  image.setAttribute("SRC", "bluedot.gif");
  image.setAttribute("WIDTH", "100");
  image.setAttribute("HEIGHT", "40");
  image.setAttribute("BORDER", "2");
  return image;
}

function makeBlock()
{
  var block = document.createElement("DIV");
  var text = document.createTextNode("Block Text");
  block.appendChild(text);
  return block;
}

function appendInline()
{
  var i = makeInline();
  var it = document.getElementById("it");
  it.appendChild(i);
}

function insertInline(index)
{
  var i = makeInline();
  var it = document.getElementById("it");
  var kids = it.childNodes;
  if ((index < 0) || (index > kids.length)) index = 0;
  var before = kids[index];
  it.insertBefore(i, before);
}

function appendBlock()
{
  var b = makeBlock();
  var it = document.getElementById("it");
  it.appendChild(b);
}

function insertBlock(index)
{
  var b = makeBlock();
  var it = document.getElementById("it");
  var kids = it.childNodes;
  if ((index < 0) || (index > kids.length)) index = 0;
  var before = kids[index];
  it.insertBefore(b, before);
}

function removeAllChildren()
{
  var it = document.getElementById("it");
  var kids = it.childNodes;
  var n = kids.length;
  for (i = 0; i < n; i++) {
   it.removeChild(kids[0]);
  }
}

function removeChild(index)
{
  var it = document.getElementById("it");
  var kids = it.childNodes;
  it.removeChild(kids[index]);
}

function testAppend()
{
  appendInline();  
  appendBlock();   
  appendInline();  
  appendBlock();   
  appendBlock();   
  appendInline();  
  removeAllChildren();
}

function testInsert()
{
  testAppend();

  
  insertInline(0); 
  insertInline(3); 
  insertInline(7); 
  insertInline(8); 

  
  insertBlock(0);  
  insertBlock(1);  
  insertInline(0);
  insertInline(0);
  insertInline(11);
  insertBlock(13); 
  insertInline(0);
  insertInline(0);
  insertBlock(2);  

  
  removeAllChildren();
  appendInline();
  insertBlock(0);  

  removeAllChildren();
  appendInline();
  appendInline();
  insertBlock(1);  

  removeAllChildren();
}

function testRemove()
{
  appendInline();
  removeChild(0);  

  appendBlock();
  removeChild(0);  

  appendInline();
  appendBlock();
  appendInline();
  appendBlock();
  appendInline();
  appendBlock();
  appendInline();

  removeChild(3);  
  removeChild(2);  
  removeChild(2);  
  removeChild(1);  
  removeAllChildren();

  appendInline();
  appendBlock();
  appendInline();
  appendInline();
  appendBlock();
  removeChild(4);  

  removeAllChildren();
}

function runTest()
{
  dump("Testing inline incremental reflow\n");
  testAppend();
  testInsert();
  testRemove();
}
