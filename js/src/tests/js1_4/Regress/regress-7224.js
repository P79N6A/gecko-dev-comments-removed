




































gTestfile = 'regress-7224.js';








var SECTION = "regress";       
var VERSION = "JS1_4"; 
var TITLE   = "Regression test for bugzilla #7224";       
var BUGNUMBER = "http://bugzilla.mozilla.org/show_bug.cgi?id=7224";     

startTest();               



















var f = new Function( "return arguments.caller" );
var o = {};

o.foo = f;
o.foo("a", "b", "c");


AddTestCase(
  "var f = new Function( 'return arguments.caller' ); f()",
  undefined,
  f() );

AddTestCase(
  "var o = {}; o.foo = f; o.foo('a')",
  undefined,
  o.foo('a') );

test();       

