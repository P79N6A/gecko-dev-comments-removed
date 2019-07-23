





































gTestfile = '11.2.1-3-n.js';











































var SECTION = "11.2.1-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Property Accessors";
writeHeaderToLog( SECTION + " "+TITLE );



var PROPERTY = new Array();
var p = 0;



PROPERTY[p++] = new Property(  "undefined",    void 0,   "undefined",   NaN );

for ( var i = 0, RESULT; i < PROPERTY.length; i++ ) {

  DESCRIPTION = PROPERTY[i].object + ".valueOf()";
  EXPECTED = "error";

  new TestCase( SECTION,
		PROPERTY[i].object + ".valueOf()",
		PROPERTY[i].value,
		eval( PROPERTY[i].object+ ".valueOf()" ) );

  new TestCase( SECTION,
		PROPERTY[i].object + ".toString()",
		PROPERTY[i].string,
		eval(PROPERTY[i].object+ ".toString()") );
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
