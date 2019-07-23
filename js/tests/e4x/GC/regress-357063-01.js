





































gTestfile = 'regress-357063-01.js';

var BUGNUMBER = 357063;
var summary = 'GC hazard in XMLEquality';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);


var xml = new XML("<xml><a>text</a><a>text</a></xml>");
var xml2 = new XML("<xml><a>text</a><a>text</a></xml>");
var list1 = xml.a;
var list2 = xml2.a;

XML.prototype.function::toString = function() {
  if (xml2) {
    delete list2[1];
    delete list2[0];
    xml2 = null;
    gc();
  }
  return "text";
}

var value = list1 == list2;

print('list1: ' + list1.toXMLString());
print('list2: ' + list2.toXMLString());
print('list1 == list2: ' + value);

TEST(1, expect, actual);

END();
