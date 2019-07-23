





































gTestfile = '15.5.4.5-6.js';































var SECTION = "15.5.4.5-6";
var VERSION = "ECMA_2";
startTest();
var TITLE   = "String.prototype.charCodeAt";

writeHeaderToLog( SECTION + " "+ TITLE);


new TestCase( SECTION,
	      "var obj = true; obj.__proto__.charCodeAt = String.prototype.charCodeAt; var s = ''; for ( var i = 0; i < 4; i++ ) s+= String.fromCharCode( obj.charCodeAt(i) ); s",
	      "true",
	      eval("var obj = true; obj.__proto__.charCodeAt = String.prototype.charCodeAt; var s = ''; for ( var i = 0; i < 4; i++ ) s+= String.fromCharCode( obj.charCodeAt(i) ); s") );

new TestCase( SECTION,
	      "var obj = 1234; obj.__proto__.charCodeAt = String.prototype.charCodeAt; var s = ''; for ( var i = 0; i < 4; i++ ) s+= String.fromCharCode( obj.charCodeAt(i) ); s",
	      "1234",
	      eval("var obj = 1234; obj.__proto__.charCodeAt = String.prototype.charCodeAt; var s = ''; for ( var i = 0; i < 4; i++ ) s+= String.fromCharCode( obj.charCodeAt(i) ); s") );

new TestCase( SECTION,
	      "var obj = 'hello'; obj.__proto__.charCodeAt = String.prototype.charCodeAt; var s = ''; for ( var i = 0; i < 5; i++ ) s+= String.fromCharCode( obj.charCodeAt(i) ); s",
	      "hello",
	      eval("var obj = 'hello'; obj.__proto__.charCodeAt = String.prototype.charCodeAt; var s = ''; for ( var i = 0; i < 5; i++ ) s+= String.fromCharCode( obj.charCodeAt(i) ); s") );

test();
