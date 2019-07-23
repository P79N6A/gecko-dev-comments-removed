




































var gTestfile = 'regress-407323.js';

var BUGNUMBER = 407323;
var summary = 'XML, XMLList, QName are mutable, Namespace is not.';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var obj           = {};
  var saveQName     = QName;
  var saveXML       = XML;
  var saveXMLList   = XMLList;
  var saveNamespace = Namespace;

  QName = obj;
  reportCompare(obj, QName, summary + ': QName');

  XML = obj;
  reportCompare(obj, XML, summary + ': XML');

  XMLList = obj;
  reportCompare(obj, XMLList, summary + ': XMLList');

  Namespace = obj;
  reportCompare(obj, Namespace, summary + ': Namespace');

  exitFunc ('test');
}
