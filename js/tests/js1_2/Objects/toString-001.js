





































gTestfile = 'toString-001.js';














var SECTION = "JS1_2";
var VERSION = "JS1_2";
startTest();
var TITLE   = "Object.toString()";

writeHeaderToLog( SECTION + " "+ TITLE);

var o = new Object();

new TestCase( SECTION,
	      "var o = new Object(); o.toString()",
	      "{}",
	      o.toString() );

o = {};

new TestCase( SECTION,
	      "o = {}; o.toString()",
	      "{}",
	      o.toString() );

o = { name:"object", length:0, value:"hello" }

  new TestCase( SECTION,
		"o = { name:\"object\", length:0, value:\"hello\" }; o.toString()",
		true,
		checkObjectToString(o.toString(), ['name:"object"', 'length:0',
						   'value:"hello"']));

o = { name:"object", length:0, value:"hello",
      toString:new Function( "return this.value+''" ) }

  new TestCase( SECTION,
		"o = { name:\"object\", length:0, value:\"hello\", "+
		"toString:new Function( \"return this.value+''\" ) }; o.toString()",
		"hello",
		o.toString() );



test();










function checkObjectToString(s, a) {
  var m = /^\{(.*)\}$/(s);
  if (!m)
    return false;	
  var a2 = m[1].split(", ");
  if (a.length != a2.length)
    return false;	
  a.sort();
  a2.sort();
  for (var i=0; i < a.length; i++) {
    if (a[i] != a2[i])
      return false;	
  }
  return true;
}

