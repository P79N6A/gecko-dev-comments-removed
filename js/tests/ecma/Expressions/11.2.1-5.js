





































gTestfile = '11.2.1-5.js';











































var SECTION = "11.2.1-5";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Property Accessors";
writeHeaderToLog( SECTION + " "+TITLE );



var PROPERTY = new Array();
var p = 0;



PROPERTY[p++] = new Property(  new String("hi"),    "hi",   "hi",   NaN );
PROPERTY[p++] = new Property(  new Number(NaN),         NaN,    "NaN",    NaN );
PROPERTY[p++] = new Property(  new Number(3),           3,      "3",    3  );
PROPERTY[p++] = new Property(  new Boolean(true),        true,      "true",    1 );
PROPERTY[p++] = new Property(  new Boolean(false),       false,      "false",    0 );

for ( var i = 0, RESULT; i < PROPERTY.length; i++ ) {
  new TestCase( SECTION,
                PROPERTY[i].object + ".valueOf()",
                PROPERTY[i].value,
                eval( "PROPERTY[i].object.valueOf()" ) );

  new TestCase( SECTION,
                PROPERTY[i].object + ".toString()",
                PROPERTY[i].string,
                eval( "PROPERTY[i].object.toString()" ) );

}

test();

function MyObject( value ) {
  this.value = value;
  this.stringValue = value +"";
  this.numberValue = Number(value);
  return this;
}
function Property( object, value, string, number ) {
  this.object = object;
  this.string = String(value);
  this.number = Number(value);
  this.value = value;
}
