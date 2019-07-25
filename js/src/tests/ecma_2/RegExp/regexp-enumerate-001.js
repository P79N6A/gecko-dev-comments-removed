






























var SECTION = "regexp-enumerate-001";
var VERSION = "ECMA_2";
var TITLE   = "Regression Test for Enumerating Properties";

var BUGNUMBER="339403";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);










var r = new RegExp();

var e = new Array();

var t = new TestRegExp();

for ( p in r ) { e[e.length] = { property:p, value:r[p] }; t.addProperty( p, r[p]) };

new TestCase( SECTION,
	      "r = new RegExp(); e = new Array(); "+
	      "for ( p in r ) { e[e.length] = { property:p, value:r[p] }; e.length",
	      0,
	      e.length );

test();

function TestRegExp() {
  this.addProperty = addProperty;
}
function addProperty(name, value) {
  var pass = false;

  if ( eval("this."+name) != void 0 ) {
    pass = true;
  } else {
    eval( "this."+ name+" = "+ false );
  }

  new TestCase( SECTION,
		"Property: " + name +" already enumerated?",
		false,
		pass );

  if ( gTestcases[ gTestcases.length-1].passed == false ) {
    gTestcases[gTestcases.length-1].reason = "property already enumerated";

  }

}
