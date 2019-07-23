





































gTestfile = 'object-001.js';






var SECTION = "java array object inheritance JavaScript Array methods";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 " + SECTION;

startTest();

dt = new Packages.com.netscape.javascript.qa.liveconnect.DataTypeClass();

obArray = dt.PUB_ARRAY_OBJECT;



new TestCase(
  "dt = new Packages.com.netscape.javascript.qa.liveconnect.DataTypeClass(); "+
  "obArray = dt.PUB_ARRAY_OBJECT" +
  "obArray.join() +''",
  join(obArray),
  obArray.join() );



new TestCase(
  "typeof obArray.reverse().join()",
  reverse(obArray),
  obArray.reverse().join() );

new TestCase(
  "obArray.reverse().getClass().getName() +''",
  "[Ljava.lang.Object;",
  obArray.reverse().getClass().getName() +'');

test();

function join( a ) {
  for ( var i = 0, s = ""; i < a.length; i++ ) {
    s += a[i].toString() + ( i + 1 < a.length ? "," : "" );
  }
  return s;
}
function reverse( a ) {
  for ( var i = a.length -1, s = ""; i >= 0; i-- ) {
    s += a[i].toString() + ( i> 0 ? "," : "" );
  }
  return s;
}
