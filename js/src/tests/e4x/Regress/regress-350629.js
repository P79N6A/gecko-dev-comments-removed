




































gTestfile = 'regress-350629.js';


var BUGNUMBER     = "350629";
var summary = ".toXMLString can include invalid generated prefixes";
var actual, expect;

printBugNumber(BUGNUMBER);
START(summary);





var failed = false;

function extractPrefix(el, attrName, attrVal)
{
  var str = el.toXMLString();
  var regex = new RegExp(' (.+?):' + attrName + '="' + attrVal + '"');
  return str.match(regex)[1];
}

function assertValidPrefix(p, msg)
{
  if (!isXMLName(p) ||
      0 == p.search(/xml/i))
    throw msg;
}

var el, n, p;

try
{
  
  el = <foo/>;
  n = new Namespace("http://foo/bar.xml");
  el.@n::fiz = "eit";
  p = extractPrefix(el, "fiz", "eit");
  assertValidPrefix(p, "namespace " + n.uri + " generated invalid prefix " + p);

  
  el = <foo/>;
  n = new Namespace("http://foo/bar.XML");
  el.@n::fiz = "eit";
  p = extractPrefix(el, "fiz", "eit");
  assertValidPrefix(p, "namespace " + n.uri + " generated invalid prefix " + p);

  
  el = <foo/>;
  n = new Namespace("http://foo/bar.xmln");
  el.@n::baz = "quux";
  p = extractPrefix(el, "baz", "quux");
  assertValidPrefix(p, "namespace " + n.uri + " generated invalid prefix " + p);


  
  el = <foo/>;
  n = new Namespace("xml:///");
  el.@n::bike = "cycle";
  p = extractPrefix(el, "bike", "cycle");
  assertValidPrefix(p, "namespace " + n.uri + " generated invalid prefix " + p);


  
  
  el = <aaaaa:foo xmlns:aaaaa="http://baz/"/>;
  n = new Namespace("xml:///");
  el.@n::bike = "cycle";
  p = extractPrefix(el, "bike", "cycle");
  assertValidPrefix(p, "namespace " + n.uri + " generated invalid prefix " + p);



  
  
  
  el = <foo/>;
  n = new Namespace(".:/.././.:/:");
  el.@n::biz = "17";
  p = extractPrefix(el, "biz", "17");
  assertValidPrefix(p, "namespace " + n.uri + " generated invalid prefix " + p);
}
catch (ex)
{
  failed = ex;
}

expect = false;
actual = failed;

TEST(1, expect, actual);
