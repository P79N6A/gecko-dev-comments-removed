
function run_test()
{

  



  
  test_getElementsByTagName();
  test_getElementsByTagNameNS();
  test_getElementsByAttribute();
  test_getElementsByAttributeNS();

  
  

}  

function test_getElementsByTagName()
{
  var doc = ParseFile("nodelist_data_1.xml");
  var root = doc.documentElement;

  
  do_check_true(doc.getElementsByTagName("*") instanceof nsIDOMNodeList);
  do_check_true(root.getElementsByTagName("*") instanceof nsIDOMNodeList);
  
  
  do_check_eq(doc.getElementsByTagName("*").length,
              root.getElementsByTagName("*").length + 1);
  do_check_eq(doc.getElementById("test2").getElementsByTagName("*").length,
              8);
  do_check_eq(doc.getElementById("test2").getElementsByTagName("test").length,
              3);

  
  
  do_check_eq(doc.getElementsByTagName("*").item(0), root);

  
  var numTests = doc.getElementsByTagName("test").length;
  do_check_eq(numTests, 5);

  for (var i = 1; i <= numTests; ++i) {
    do_check_true(doc.getElementById("test" + i) instanceof nsIDOMElement);
    do_check_eq(doc.getElementById("test" + i),
                doc.getElementsByTagName("test").item(i-1));
  }

  
  do_check_true(doc.getElementsByTagName("foo:test")
                instanceof nsIDOMNodeList);
  do_check_eq(doc.getElementsByTagName("foo:test").length, 2);

  do_check_true(doc.getElementsByTagName("foo2:test")
                instanceof nsIDOMNodeList);
  do_check_eq(doc.getElementsByTagName("foo2:test").length, 3);

  do_check_true(doc.getElementsByTagName("bar:test")
                instanceof nsIDOMNodeList);
  do_check_eq(doc.getElementsByTagName("bar:test").length, 4);
}

function test_getElementsByTagNameNS()
{
  var doc = ParseFile("nodelist_data_1.xml");
  var root = doc.documentElement;

  
  do_check_true(doc.getElementsByTagNameNS("*", "*") instanceof nsIDOMNodeList);
  do_check_true(root.getElementsByTagNameNS("*", "*") instanceof nsIDOMNodeList);

  
  var list1 = doc.getElementsByTagNameNS("", "test");
  var list2 = doc.getElementsByTagNameNS(null, "test");
  do_check_eq(list1.length, list2.length);
  for (var i = 0; i < list1.length; ++i) {
    do_check_eq(list1.item(i), list2.item(i));
  }
  
  
  do_check_eq(doc.getElementsByTagNameNS("*", "*").length,
              root.getElementsByTagNameNS("*", "*").length + 1);
  do_check_eq(doc.getElementById("test2")
                 .getElementsByTagNameNS("*", "*").length,
              8);
  do_check_eq(doc.getElementById("test2")
                 .getElementsByTagNameNS("", "test").length,
              1);
  do_check_eq(doc.getElementById("test2")
                 .getElementsByTagNameNS("*", "test").length,
              7);

  
  
  do_check_eq(doc.getElementsByTagNameNS("*", "*").item(0), root);
  do_check_eq(doc.getElementsByTagNameNS(null, "*").item(0), root);

  

     
  var numTests = doc.getElementsByTagNameNS("*", "test").length;
  do_check_eq(numTests, 14);

  for (var i = 1; i <= numTests; ++i) {
    do_check_true(doc.getElementById("test" + i) instanceof nsIDOMElement);
    do_check_eq(doc.getElementById("test" + i),
                doc.getElementsByTagNameNS("*", "test").item(i-1));
  }

  
  var test2 = doc.getElementById("test2");
  do_check_eq(doc.getElementsByTagNameNS("", "test").length,
              3);
  do_check_eq(test2.getElementsByTagNameNS("", "test").length,
              1);
  do_check_eq(doc.getElementsByTagNameNS("foo", "test").length,
              7);
  do_check_eq(test2.getElementsByTagNameNS("foo", "test").length,
              4);
  do_check_eq(doc.getElementsByTagNameNS("foo2", "test").length,
              0);
  do_check_eq(test2.getElementsByTagNameNS("foo2", "test").length,
              0);
  do_check_eq(doc.getElementsByTagNameNS("bar", "test").length,
              4);
  do_check_eq(test2.getElementsByTagNameNS("bar", "test").length,
              2);

  
  do_check_true(doc.getElementsByTagNameNS(null, "foo:test")
                instanceof nsIDOMNodeList);
  do_check_eq(doc.getElementsByTagNameNS(null, "foo:test").length, 0);
  do_check_eq(doc.getElementsByTagNameNS("foo", "foo:test").length, 0);
  do_check_eq(doc.getElementsByTagNameNS("bar", "foo:test").length, 0);
  do_check_eq(doc.getElementsByTagNameNS("*", "foo:test").length, 0);
  
  do_check_true(doc.getElementsByTagNameNS(null, "foo2:test")
                instanceof nsIDOMNodeList);
  do_check_eq(doc.getElementsByTagNameNS(null, "foo2:test").length, 0);
  do_check_eq(doc.getElementsByTagNameNS("foo2", "foo2:test").length, 0);
  do_check_eq(doc.getElementsByTagNameNS("bar", "foo2:test").length, 0);
  do_check_eq(doc.getElementsByTagNameNS("*", "foo2:test").length, 0);
  
  do_check_true(doc.getElementsByTagNameNS(null, "bar:test")
                instanceof nsIDOMNodeList);
  do_check_eq(doc.getElementsByTagNameNS(null, "bar:test").length, 0);
  do_check_eq(doc.getElementsByTagNameNS("bar", "bar:test").length, 0);
  do_check_eq(doc.getElementsByTagNameNS("*", "bar:test").length, 0);

  
  
  
  
  list1 = doc.getElementsByTagNameNS("random-bogus-namespace", "foo");
  list2 = doc.documentElement.getElementsByTagNameNS("random-bogus-namespace2",
                                                     "foo");
  do_check_neq(list1, list2);
  do_check_eq(list1.length, 0);
  do_check_eq(list2.length, 0);
  var newNode = doc.createElementNS("random-bogus-namespace", "foo");
  doc.documentElement.appendChild(newNode);
  var newNode = doc.createElementNS("random-bogus-namespace2", "foo");
  doc.documentElement.appendChild(newNode);
  do_check_eq(list1.length, 1);  
  do_check_eq(list2.length, 1);  
}

function test_getElementsByAttribute()
{
  var doc = ParseFile("nodelist_data_2.xul");
  var root = doc.documentElement;

  
  

  do_check_true(root instanceof nsIDOMXULElement);

  do_check_true(root.getElementsByAttribute("foo", "foo") instanceof
                nsIDOMNodeList);

  var master1 = doc.getElementById("master1");
  var master2 = doc.getElementById("master2");
  var master3 = doc.getElementById("master3");
  var external = doc.getElementById("external");

  do_check_true(master1 instanceof nsIDOMXULElement);
  do_check_true(master2 instanceof nsIDOMXULElement);
  do_check_true(master3 instanceof nsIDOMXULElement);
  do_check_true(external instanceof nsIDOMXULElement);

  
  do_check_eq(root.getElementsByAttribute("foo", "foo").length,
              14);
  do_check_eq(master1.getElementsByAttribute("foo", "foo").length,
              4);

  do_check_eq(root.getElementsByAttribute("foo", "bar").length,
              7);
  do_check_eq(master1.getElementsByAttribute("foo", "bar").length,
              2);

  do_check_eq(root.getElementsByAttribute("bar", "bar").length,
              7);
  do_check_eq(master1.getElementsByAttribute("bar", "bar").length,
              2);

  do_check_eq(root.getElementsByAttribute("foo", "*").length,
              21);
  do_check_eq(master1.getElementsByAttribute("foo", "*").length,
              6);

  
  do_check_eq(root.getElementsByAttribute("foo:foo", "foo").length,
              16);
  do_check_eq(master1.getElementsByAttribute("foo:foo", "foo").length,
              5);
  do_check_eq(master2.getElementsByAttribute("foo:foo", "foo").length,
              4);
  do_check_eq(master3.getElementsByAttribute("foo:foo", "foo").length,
              4);
  do_check_eq(external.getElementsByAttribute("foo:foo", "foo").length,
              2);

  do_check_eq(root.getElementsByAttribute("foo:foo", "bar").length,
              9);
  do_check_eq(master1.getElementsByAttribute("foo:foo", "bar").length,
              2);
  do_check_eq(master2.getElementsByAttribute("foo:foo", "bar").length,
              3);
  do_check_eq(master3.getElementsByAttribute("foo:foo", "bar").length,
              2);
  do_check_eq(external.getElementsByAttribute("foo:foo", "bar").length,
              1);

  do_check_eq(root.getElementsByAttribute("foo:bar", "foo").length,
              7);
  do_check_eq(master1.getElementsByAttribute("foo:bar", "foo").length,
              2);
  do_check_eq(master2.getElementsByAttribute("foo:bar", "foo").length,
              2);
  do_check_eq(master3.getElementsByAttribute("foo:bar", "foo").length,
              2);
  do_check_eq(external.getElementsByAttribute("foo:bar", "foo").length,
              1);

  do_check_eq(root.getElementsByAttribute("foo:bar", "bar").length,
              14);
  do_check_eq(master1.getElementsByAttribute("foo:bar", "bar").length,
              4);
  do_check_eq(master2.getElementsByAttribute("foo:bar", "bar").length,
              4);
  do_check_eq(master3.getElementsByAttribute("foo:bar", "bar").length,
              4);
  do_check_eq(external.getElementsByAttribute("foo:bar", "bar").length,
              2);

  do_check_eq(root.getElementsByAttribute("foo2:foo", "foo").length,
              8);
  do_check_eq(master1.getElementsByAttribute("foo2:foo", "foo").length,
              2);
  do_check_eq(master2.getElementsByAttribute("foo2:foo", "foo").length,
              2);
  do_check_eq(master3.getElementsByAttribute("foo2:foo", "foo").length,
              3);
  do_check_eq(external.getElementsByAttribute("foo2:foo", "foo").length,
              1);

  do_check_eq(root.getElementsByAttribute("foo:foo", "*").length,
              25);
  do_check_eq(master1.getElementsByAttribute("foo:foo", "*").length,
              7);
  do_check_eq(master2.getElementsByAttribute("foo:foo", "*").length,
              7);
  do_check_eq(master3.getElementsByAttribute("foo:foo", "*").length,
              6);
  do_check_eq(external.getElementsByAttribute("foo:foo", "*").length,
              3);

  do_check_eq(root.getElementsByAttribute("foo2:foo", "bar").length,
              0);
  do_check_eq(root.getElementsByAttribute("foo:foo", "baz").length,
              0);
}

function test_getElementsByAttributeNS()
{
  var doc = ParseFile("nodelist_data_2.xul");
  var root = doc.documentElement;

  
  

  do_check_true(root instanceof nsIDOMXULElement);

  
  do_check_true(root.getElementsByAttributeNS("*", "*", "*") instanceof
                nsIDOMNodeList);

  var master1 = doc.getElementById("master1");
  var master2 = doc.getElementById("master2");
  var master3 = doc.getElementById("master3");
  var external = doc.getElementById("external");

  do_check_true(master1 instanceof nsIDOMXULElement);
  do_check_true(master2 instanceof nsIDOMXULElement);
  do_check_true(master3 instanceof nsIDOMXULElement);
  do_check_true(external instanceof nsIDOMXULElement);
  
  
  do_check_eq(root.getElementsByAttributeNS("*", "foo", "foo").length,
              38);
  do_check_eq(master1.getElementsByAttributeNS("*", "foo", "foo").length,
              11);
  do_check_eq(master2.getElementsByAttributeNS("*", "foo", "foo").length,
              10);
  do_check_eq(master3.getElementsByAttributeNS("*", "foo", "foo").length,
              11);

  do_check_eq(root.getElementsByAttributeNS("*", "foo", "bar").length,
              16);
  do_check_eq(master1.getElementsByAttributeNS("*", "foo", "bar").length,
              4);
  do_check_eq(master2.getElementsByAttributeNS("*", "foo", "bar").length,
              5);
  do_check_eq(master3.getElementsByAttributeNS("*", "foo", "bar").length,
              4);

  do_check_eq(root.getElementsByAttributeNS("*", "bar", "bar").length,
              21);
  do_check_eq(master1.getElementsByAttributeNS("*", "bar", "bar").length,
              6);
  do_check_eq(master2.getElementsByAttributeNS("*", "bar", "bar").length,
              6);
  do_check_eq(master3.getElementsByAttributeNS("*", "bar", "bar").length,
              6);

  do_check_eq(root.getElementsByAttributeNS("*", "foo", "*").length,
              54);
  do_check_eq(master1.getElementsByAttributeNS("*", "foo", "*").length,
              15);
  do_check_eq(master2.getElementsByAttributeNS("*", "foo", "*").length,
              15);
  do_check_eq(master3.getElementsByAttributeNS("*", "foo", "*").length,
              15);

  
  do_check_eq(root.getElementsByAttributeNS("", "foo", "foo").length,
              root.getElementsByAttribute("foo", "foo").length);
  do_check_eq(master1.getElementsByAttributeNS("", "foo", "foo").length,
              master1.getElementsByAttribute("foo", "foo").length);
  do_check_eq(master2.getElementsByAttributeNS("", "foo", "foo").length,
              master2.getElementsByAttribute("foo", "foo").length);
  do_check_eq(master3.getElementsByAttributeNS("", "foo", "foo").length,
              master3.getElementsByAttribute("foo", "foo").length);
  
  
  do_check_eq(root.getElementsByAttributeNS("foo", "foo", "foo").length,
              24);
  do_check_eq(master1.getElementsByAttributeNS("foo", "foo", "foo").length,
              7);
  do_check_eq(master2.getElementsByAttributeNS("foo", "foo", "foo").length,
              6);
  do_check_eq(master3.getElementsByAttributeNS("foo", "foo", "foo").length,
              7);

  do_check_eq(root.getElementsByAttributeNS("foo", "foo", "bar").length,
              9);
  do_check_eq(master1.getElementsByAttributeNS("foo", "foo", "bar").length,
              2);
  do_check_eq(master2.getElementsByAttributeNS("foo", "foo", "bar").length,
              3);
  do_check_eq(master3.getElementsByAttributeNS("foo", "foo", "bar").length,
              2);

  do_check_eq(root.getElementsByAttributeNS("foo", "bar", "foo").length,
              7);
  do_check_eq(master1.getElementsByAttributeNS("foo", "bar", "foo").length,
              2);
  do_check_eq(master2.getElementsByAttributeNS("foo", "bar", "foo").length,
              2);
  do_check_eq(master3.getElementsByAttributeNS("foo", "bar", "foo").length,
              2);

  do_check_eq(root.getElementsByAttributeNS("foo", "bar", "bar").length,
              14);
  do_check_eq(master1.getElementsByAttributeNS("foo", "bar", "bar").length,
              4);
  do_check_eq(master2.getElementsByAttributeNS("foo", "bar", "bar").length,
              4);
  do_check_eq(master3.getElementsByAttributeNS("foo", "bar", "bar").length,
              4);
}
