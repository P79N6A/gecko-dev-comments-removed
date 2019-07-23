





































gTestfile = '15.5.4.7-2.js';









































var SECTION = "15.5.4.7-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.protoype.lastIndexOf";

writeHeaderToLog( SECTION + " "+ TITLE);


new TestCase( SECTION, "String.prototype.lastIndexOf.length",           1,          String.prototype.lastIndexOf.length );
new TestCase( SECTION, "delete String.prototype.lastIndexOf.length",    false,      delete String.prototype.lastIndexOf.length );
new TestCase( SECTION, "delete String.prototype.lastIndexOf.length; String.prototype.lastIndexOf.length",   1,  eval("delete String.prototype.lastIndexOf.length; String.prototype.lastIndexOf.length" ) );

new TestCase( SECTION, "var s = new String(''); s.lastIndexOf('', 0)",          LastIndexOf("","",0),  eval("var s = new String(''); s.lastIndexOf('', 0)") );
new TestCase( SECTION, "var s = new String(''); s.lastIndexOf('')",             LastIndexOf("",""),  eval("var s = new String(''); s.lastIndexOf('')") );
new TestCase( SECTION, "var s = new String('hello'); s.lastIndexOf('', 0)",     LastIndexOf("hello","",0),  eval("var s = new String('hello'); s.lastIndexOf('',0)") );
new TestCase( SECTION, "var s = new String('hello'); s.lastIndexOf('')",        LastIndexOf("hello",""),  eval("var s = new String('hello'); s.lastIndexOf('')") );

new TestCase( SECTION, "var s = new String('hello'); s.lastIndexOf('ll')",     LastIndexOf("hello","ll"),  eval("var s = new String('hello'); s.lastIndexOf('ll')") );
new TestCase( SECTION, "var s = new String('hello'); s.lastIndexOf('ll', 0)",  LastIndexOf("hello","ll",0),  eval("var s = new String('hello'); s.lastIndexOf('ll', 0)") );
new TestCase( SECTION, "var s = new String('hello'); s.lastIndexOf('ll', 1)",  LastIndexOf("hello","ll",1),  eval("var s = new String('hello'); s.lastIndexOf('ll', 1)") );
new TestCase( SECTION, "var s = new String('hello'); s.lastIndexOf('ll', 2)",  LastIndexOf("hello","ll",2),  eval("var s = new String('hello'); s.lastIndexOf('ll', 2)") );
new TestCase( SECTION, "var s = new String('hello'); s.lastIndexOf('ll', 3)",  LastIndexOf("hello","ll",3),  eval("var s = new String('hello'); s.lastIndexOf('ll', 3)") );
new TestCase( SECTION, "var s = new String('hello'); s.lastIndexOf('ll', 4)",  LastIndexOf("hello","ll",4),  eval("var s = new String('hello'); s.lastIndexOf('ll', 4)") );
new TestCase( SECTION, "var s = new String('hello'); s.lastIndexOf('ll', 5)",  LastIndexOf("hello","ll",5),  eval("var s = new String('hello'); s.lastIndexOf('ll', 5)") );
new TestCase( SECTION, "var s = new String('hello'); s.lastIndexOf('ll', 6)",  LastIndexOf("hello","ll",6),  eval("var s = new String('hello'); s.lastIndexOf('ll', 6)") );

new TestCase( SECTION, "var s = new String('hello'); s.lastIndexOf('ll', 1.5)", LastIndexOf('hello','ll', 1.5), eval("var s = new String('hello'); s.lastIndexOf('ll', 1.5)") );
new TestCase( SECTION, "var s = new String('hello'); s.lastIndexOf('ll', 2.5)", LastIndexOf('hello','ll', 2.5),  eval("var s = new String('hello'); s.lastIndexOf('ll', 2.5)") );
new TestCase( SECTION, "var s = new String('hello'); s.lastIndexOf('ll', -1)",  LastIndexOf('hello','ll', -1), eval("var s = new String('hello'); s.lastIndexOf('ll', -1)") );
new TestCase( SECTION, "var s = new String('hello'); s.lastIndexOf('ll', -1.5)",LastIndexOf('hello','ll', -1.5), eval("var s = new String('hello'); s.lastIndexOf('ll', -1.5)") );

new TestCase( SECTION, "var s = new String('hello'); s.lastIndexOf('ll', -Infinity)",    LastIndexOf("hello","ll",-Infinity), eval("var s = new String('hello'); s.lastIndexOf('ll', -Infinity)") );
new TestCase( SECTION, "var s = new String('hello'); s.lastIndexOf('ll', Infinity)",    LastIndexOf("hello","ll",Infinity), eval("var s = new String('hello'); s.lastIndexOf('ll', Infinity)") );
new TestCase( SECTION, "var s = new String('hello'); s.lastIndexOf('ll', NaN)",    LastIndexOf("hello","ll",NaN), eval("var s = new String('hello'); s.lastIndexOf('ll', NaN)") );
new TestCase( SECTION, "var s = new String('hello'); s.lastIndexOf('ll', -0)",    LastIndexOf("hello","ll",-0), eval("var s = new String('hello'); s.lastIndexOf('ll', -0)") );
for ( var i = 0; i < ( "[object Object]" ).length; i++ ) {
  new TestCase(   SECTION,
		  "var o = new Object(); o.lastIndexOf = String.prototype.lastIndexOf; o.lastIndexOf('b', "+ i + ")",
		  ( i < 2 ? -1 : ( i < 9  ? 2 : 9 )) ,
		  eval("var o = new Object(); o.lastIndexOf = String.prototype.lastIndexOf; o.lastIndexOf('b', "+ i + ")") );
}
for ( var i = 0; i < 5; i ++ ) {
  new TestCase(   SECTION,
		  "var b = new Boolean(); b.lastIndexOf = String.prototype.lastIndexOf; b.lastIndexOf('l', "+ i + ")",
		  ( i < 2 ? -1 : 2 ),
		  eval("var b = new Boolean(); b.lastIndexOf = String.prototype.lastIndexOf; b.lastIndexOf('l', "+ i + ")") );
}
for ( var i = 0; i < 5; i ++ ) {
  new TestCase(   SECTION,
		  "var b = new Boolean(); b.toString = Object.prototype.toString; b.lastIndexOf = String.prototype.lastIndexOf; b.lastIndexOf('o', "+ i + ")",
		  ( i < 1 ? -1 : ( i < 9 ? 1 : ( i < 10 ? 9 : 10 ) ) ),
		  eval("var b = new Boolean();  b.toString = Object.prototype.toString; b.lastIndexOf = String.prototype.lastIndexOf; b.lastIndexOf('o', "+ i + ")") );
}
for ( var i = 0; i < 9; i++ ) {
  new TestCase(   SECTION,
		  "var n = new Number(Infinity); n.lastIndexOf = String.prototype.lastIndexOf; n.lastIndexOf( 'i', " + i + " )",
		  ( i < 3 ? -1 : ( i < 5 ? 3 : 5 ) ),
		  eval("var n = new Number(Infinity); n.lastIndexOf = String.prototype.lastIndexOf; n.lastIndexOf( 'i', " + i + " )") );
}
var a = new Array( "abc","def","ghi","jkl","mno","pqr","stu","vwx","yz" );

for ( var i = 0; i < (a.toString()).length; i++ ) {
  new TestCase( SECTION,
		"var a = new Array( 'abc','def','ghi','jkl','mno','pqr','stu','vwx','yz' ); a.lastIndexOf = String.prototype.lastIndexOf; a.lastIndexOf( ',mno,p', "+i+" )",
		( i < 15 ? -1 : 15 ),
		eval("var a = new Array( 'abc','def','ghi','jkl','mno','pqr','stu','vwx','yz' ); a.lastIndexOf = String.prototype.lastIndexOf; a.lastIndexOf( ',mno,p', "+i+" )") );
}

for ( var i = 0; i < 15; i ++ ) {
  new TestCase(   SECTION,
		  "var m = Math; m.lastIndexOf = String.prototype.lastIndexOf; m.lastIndexOf('t', "+ i + ")",
		  ( i < 6 ? -1 : ( i < 10 ? 6 : 10 ) ),
		  eval("var m = Math; m.lastIndexOf = String.prototype.lastIndexOf; m.lastIndexOf('t', "+ i + ")") );
}









test();

function LastIndexOf( string, search, position ) {
  string = String( string );
  search = String( search );

  position = Number( position )

    if ( isNaN( position ) ) {
      position = Infinity;
    } else {
      position = ToInteger( position );
    }

  result5= string.length;
  result6 = Math.min(Math.max(position, 0), result5);
  result7 = search.length;

  if (result7 == 0) {
    return Math.min(position, result5);
  }

  result8 = -1;

  for ( k = 0; k <= result6; k++ ) {
    if ( k+ result7 > result5 ) {
      break;
    }
    for ( j = 0; j < result7; j++ ) {
      if ( string.charAt(k+j) != search.charAt(j) ){
	break;
      }   else  {
	if ( j == result7 -1 ) {
	  result8 = k;
	}
      }
    }
  }

  return result8;
}
function ToInteger( n ) {
  n = Number( n );
  if ( isNaN(n) ) {
    return 0;
  }
  if ( Math.abs(n) == 0 || Math.abs(n) == Infinity ) {
    return n;
  }

  var sign = ( n < 0 ) ? -1 : 1;

  return ( sign * Math.floor(Math.abs(n)) );
}
