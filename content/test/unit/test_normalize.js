




































function run_test()
{
  



  init();

  test_element();

  
  
  
  
}



var doc; 

function init()
{
  doc = ParseFile("empty_document.xml");
}

function test_element()
{
  var x = doc.createElement("funk");

  
  x.appendChild(doc.createTextNode(""));
  do_check_eq(x.childNodes.length, 1);

  x.normalize();
  do_check_eq(x.childNodes.length, 0);


  
  x.appendChild(doc.createTextNode(""));
  x.appendChild(doc.createTextNode(""));
  do_check_eq(x.childNodes.length, 2);

  x.normalize();
  do_check_eq(x.childNodes.length, 0);


  
  x.appendChild(doc.createTextNode(""));
  x.appendChild(doc.createTextNode("Guaraldi"));
  do_check_eq(x.childNodes.length, 2);

  x.normalize();
  do_check_eq(x.childNodes.length, 1);
  do_check_eq(x.childNodes.item(0).nodeValue, "Guaraldi");


  
  clearKids(x);
  x.appendChild(doc.createTextNode("Guaraldi"));
  x.appendChild(doc.createTextNode(""));
  do_check_eq(x.childNodes.length, 2);

  x.normalize();
  do_check_eq(x.childNodes.item(0).nodeValue, "Guaraldi");


  
  clearKids(x);
  x.appendChild(doc.createTextNode("Guaraldi"));
  x.appendChild(doc.createTextNode(""));
  x.appendChild(doc.createElement("jazzy"));
  do_check_eq(x.childNodes.length, 3);

  x.normalize();
  do_check_eq(x.childNodes.length, 2);
  do_check_eq(x.childNodes.item(0).nodeValue, "Guaraldi");
  do_check_eq(x.childNodes.item(1).nodeName, "jazzy");


  
  clearKids(x);
  var kid = doc.createElement("eit");
  kid.appendChild(doc.createTextNode(""));

  x.appendChild(doc.createTextNode("Guaraldi"));
  x.appendChild(doc.createTextNode(""));
  x.appendChild(kid);
  do_check_eq(x.childNodes.length, 3);
  do_check_eq(x.childNodes.item(2).childNodes.length, 1);

  x.normalize();
  do_check_eq(x.childNodes.length, 2);
  do_check_eq(x.childNodes.item(0).nodeValue, "Guaraldi");
  do_check_eq(x.childNodes.item(1).childNodes.length, 0);
}




function clearKids(node)
{
  while (node.hasChildNodes())
    node.removeChild(node.childNodes.item(0));
}
