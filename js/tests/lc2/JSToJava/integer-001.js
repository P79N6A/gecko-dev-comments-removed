





































gTestfile = 'integer-001.js';








































var SECTION = "LiveConnect";
var VERSION = "1_3";
var TITLE   = "LiveConnect JavaScript to Java Data Type Conversion";

var BUGNUMBER="196276";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);


var E_TYPE = "object";



var E_JSCLASS = "[object JavaObject]";


var a = new Array();
var i = 0;


a[i++] = new TestObject( "new java.lang.Integer(\"0.\")",
			 new java.lang.Integer("0"), 0 );

a[i++] =  new TestObject( "new java.lang.Integer( 5 )",
			  new java.lang.Integer(5), 5 );

a[i++] = new TestObject( "new java.lang.Integer( 5.5 )",
			 new java.lang.Integer(5.5), 5 );

a[i++] =  new TestObject( "new java.lang.Integer( Number(5) )",
			  new java.lang.Integer(Number(5)), 5 );

a[i++] = new TestObject( "new java.lang.Integer( Number(5.5) )",
			 new java.lang.Integer(Number(5.5)), 5 );

for ( var i = 0; i < a.length; i++ ) {

  
  new TestCase(
    SECTION,
    "typeof (" + a[i].description +")",
    a[i].type,
    typeof a[i].javavalue );








  
  new TestCase(
    SECTION,
    "Number(" + a[i].description +")",
    a[i].jsvalue,
    Number( a[i].javavalue ) );
}

test();

function TestObject( description, javavalue, jsvalue ) {
  this.description = description;
  this.javavalue = javavalue;
  this.jsvalue = jsvalue;
  this.type = E_TYPE;



  return this;
}
