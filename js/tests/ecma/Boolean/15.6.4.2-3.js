





































gTestfile = '15.6.4.2-3.js';
















var SECTION = "15.6.4.2-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Boolean.prototype.toString()"
  writeHeaderToLog( SECTION + TITLE );

new TestCase( SECTION, "tostr=Boolean.prototype.toString; x=true; x.toString=tostr;x.toString()", "true", eval("tostr=Boolean.prototype.toString; x=true; x.toString=tostr;x.toString()") );
new TestCase( SECTION, "tostr=Boolean.prototype.toString; x=false; x.toString=tostr;x.toString()", "false", eval("tostr=Boolean.prototype.toString; x=false; x.toString=tostr;x.toString()") );

test();
