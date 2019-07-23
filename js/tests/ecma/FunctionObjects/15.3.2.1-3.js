





































gTestfile = '15.3.2.1-3.js';


















var SECTION = "15.3.2.1-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The Function Constructor";

writeHeaderToLog( SECTION + " "+ TITLE);

var args = "";

for ( var i = 0; i < 2000; i++ ) {
  args += "arg"+i;
  if ( i != 1999 ) {
    args += ",";
  }
}

var s = "";

for ( var i = 0; i < 2000; i++ ) {
  s += ".0005";
  if ( i != 1999 ) {
    s += ",";
  }
}

MyFunc = new Function( args, "var r=0; for (var i = 0; i < MyFunc.length; i++ ) { if ( eval('arg'+i) == void 0) break; else r += eval('arg'+i); }; return r");
MyObject = new Function( args, "for (var i = 0; i < MyFunc.length; i++ ) { if ( eval('arg'+i) == void 0) break; eval('this.arg'+i +'=arg'+i); };");

new TestCase( SECTION, "MyFunc.length",                       2000,         MyFunc.length );
new TestCase( SECTION, "var MY_OB = eval('MyFunc(s)')",       1,            eval("var MY_OB = MyFunc("+s+"); MY_OB") );

new TestCase( SECTION, "MyObject.length",                       2000,         MyObject.length );

new TestCase( SECTION, "FUN1 = new Function( 'a','b','c', 'return FUN1.length' ); FUN1.length",     3, eval("FUN1 = new Function( 'a','b','c', 'return FUN1.length' ); FUN1.length") );
new TestCase( SECTION, "FUN1 = new Function( 'a','b','c', 'return FUN1.length' ); FUN1()",          3, eval("FUN1 = new Function( 'a','b','c', 'return FUN1.length' ); FUN1()") );
new TestCase( SECTION, "FUN1 = new Function( 'a','b','c', 'return FUN1.length' ); FUN1(1,2,3,4,5)", 3, eval("FUN1 = new Function( 'a','b','c', 'return FUN1.length' ); FUN1(1,2,3,4,5)") );

test();
