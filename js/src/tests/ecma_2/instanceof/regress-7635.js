












var SECTION = "instanceof";       
var VERSION = "ECMA_2"; 
var TITLE   = "Regression test for Bugzilla #7635";       
var BUGNUMBER = "7635";     

startTest();               



















function Foo() {}
theproto = {};
Foo.prototype = theproto
  theproto instanceof Foo


  AddTestCase( "function Foo() {}; theproto = {}; Foo.prototype = theproto; theproto instanceof Foo",
	       false,
	       theproto instanceof Foo );

var f = new Function();

AddTestCase( "var f = new Function(); f instanceof f", false, f instanceof f );


test();       

