



























var SECTION = "instanceof-003";
var VERSION = "ECMA_2";
var TITLE   = "instanceof operator";
var BUGNUMBER ="7635";

startTest();

function Foo() {};
theproto = {};
Foo.prototype = theproto;

AddTestCase(
  "function Foo() = {}; theproto = {}; Foo.prototype = theproto; " +
  "theproto instanceof Foo",
  false,
  theproto instanceof Foo );


var o = {};


try
{
  AddTestCase(
    "o = {}; o instanceof o",
    "error",
    o instanceof o );
}
catch(e)
{
  AddTestCase(
    "o = {}; o instanceof o",
    "error",
    "error" );
}

test();
