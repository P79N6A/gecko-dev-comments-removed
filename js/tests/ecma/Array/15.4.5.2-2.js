





































gTestfile = '15.4.5.2-2.js';

















var SECTION = "15.4.5.2-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Array.length";

writeHeaderToLog( SECTION + " "+ TITLE);

addCase( new Array(), 0, Math.pow(2,14), Math.pow(2,14) );

addCase( new Array(), 0, 1, 1 );

addCase( new Array(Math.pow(2,12)), Math.pow(2,12), 0, 0 );
addCase( new Array(Math.pow(2,13)), Math.pow(2,13), Math.pow(2,12), Math.pow(2,12) );
addCase( new Array(Math.pow(2,12)), Math.pow(2,12), Math.pow(2,12), Math.pow(2,12) );
addCase( new Array(Math.pow(2,14)), Math.pow(2,14), Math.pow(2,12), Math.pow(2,12) )



  for ( var arg = "", i = 0; i < Math.pow(2,12); i++ ) {
    arg +=  String(i) + ( i != Math.pow(2,12)-1 ? "," : "" );

  }


var a = eval( "new Array("+arg+")" );

addCase( a, i, i, i );
addCase( a, i, Math.pow(2,12)+i+1, Math.pow(2,12)+i+1, true );
addCase( a, Math.pow(2,12)+5, 0, 0, true );

test();

function addCase( object, old_len, set_len, new_len, checkitems ) {
  object.length = set_len;

  new TestCase( SECTION,
		"array = new Array("+ old_len+"); array.length = " + set_len +
		"; array.length",
		new_len,
		object.length );

  if ( checkitems ) {
    
    if ( new_len < old_len ) {
      var passed = true;
      for ( var i = new_len; i < old_len; i++ ) {
	if ( object[i] != void 0 ) {
	  passed = false;
	}
      }
      new TestCase( SECTION,
		    "verify that array items have been deleted",
		    true,
		    passed );
    }
    if ( new_len > old_len ) {
      var passed = true;
      for ( var i = old_len; i < new_len; i++ ) {
	if ( object[i] != void 0 ) {
	  passed = false;
	}
      }
      new TestCase( SECTION,
		    "verify that new items are undefined",
		    true,
		    passed );
    }
  }

}

