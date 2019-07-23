





































gTestfile = 'regress-357063-02.js';

var BUGNUMBER = 357063;
var summary = 'GC hazard in XMLEquality';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

var xml1 = new XML("<xml>text<a>B</a></xml>");
var xml2 = new XML("<xml>text<a>C</a></xml>");

XML.prototype.function::toString = function() {
  if (xml2) {
    delete xml2.*;
    xml2 = null;
    gc();
  }
  return "text";
}

print('xml1: ' + xml1);
print('xml2: ' + xml2);

if (xml1 == xml2)
  throw "unexpected result";

TEST(1, expect, actual);

END();
