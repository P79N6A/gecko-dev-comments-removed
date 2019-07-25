

















var SECTION = "forin-002";
var VERSION = "ECMA_2";
var TITLE   = "The for...in  statement";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

function MyObject( value ) {
  this.value = value;
  this.valueOf = new Function ( "return this.value" );
  this.toString = new Function ( "return this.value + \"\"" );
  this.toNumber = new Function ( "return this.value + 0" );
  this.toBoolean = new Function ( "return Boolean( this.value )" );
}

ForIn_1(this);
ForIn_2(this);

ForIn_1(new MyObject(true));
ForIn_2(new MyObject(new Boolean(true)));

ForIn_2(3);

test();





function ForIn_1( object) {
  with ( object ) {
    for ( property in object ) {
      new TestCase(
	SECTION,
	"with loop in a for...in loop.  ("+object+")["+property +"] == "+
	"eval ( " + property +" )",
	true,
	object[property] == eval(property) );
    }
  }
}





function ForIn_2(object) {
  for ( property in object ) {
    with ( object ) {
      new TestCase(
	SECTION,
	"with loop in a for...in loop.  ("+object+")["+property +"] == "+
	"eval ( " + property +" )",
	true,
	object[property] == eval(property) );
    }
  }
}

