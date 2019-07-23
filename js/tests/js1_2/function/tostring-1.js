





































gTestfile = 'tostring-1.js';
















var SECTION = "tostring-1";
var VERSION = "JS1_2";
startTest();
var TITLE   = "Function.toString()";

writeHeaderToLog( SECTION + " "+ TITLE);

var tab = "    ";

t1 = new TestFunction( "stub", "value", tab + "return value;" );

t2 = new TestFunction( "ToString", "object", tab+"return object + \"\";" );

t3 = new TestFunction( "Add", "a, b, c, d, e",  tab +"var s = a + b + c + d + e;\n" +
		       tab + "return s;" );

t4 = new TestFunction( "noop", "value" );

t5 = new TestFunction( "anonymous", "", tab+"return \"hello!\";" );

var f = new Function( "return \"hello!\"");

new TestCase( SECTION,
	      "stub.toString()",
	      t1.valueOf(),
	      stub.toString() );

new TestCase( SECTION,
	      "ToString.toString()",
	      t2.valueOf(),
	      ToString.toString() );

new TestCase( SECTION,
	      "Add.toString()",
	      t3.valueOf(),
	      Add.toString() );

new TestCase( SECTION,
	      "noop.toString()",
	      t4.toString(),
	      noop.toString() );

new TestCase( SECTION,
	      "f.toString()",
	      t5.toString(),
	      f.toString() );
test();

function noop( value ) {
}
function Add( a, b, c, d, e ) {
  var s = a + b + c + d + e;
  return s;
}
function stub( value ) {
  return value;
}
function ToString( object ) {
  return object + "";
}

function ToBoolean( value ) {
  if ( value == 0 || value == NaN || value == false ) {
    return false;
  } else {
    return true;
  }
}

function TestFunction( name, args, body ) {
  if ( name == "anonymous" && version() == 120 ) {
    name = "";
  }

  this.name = name;
  this.arguments = args.toString();
  this.body = body;

  




  this.value = "function " + (name ? name : "" )+
    "("+args+") {\n"+ (( body ) ? body +"\n" : "") + "}";

  this.toString = new Function( "return this.value" );
  this.valueOf = new Function( "return this.value" );
  return this;
}
