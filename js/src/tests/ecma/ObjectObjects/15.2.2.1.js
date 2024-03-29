



























var SECTION = "15.2.2.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "new Object( value )";

writeHeaderToLog( SECTION + " "+ TITLE);


new TestCase( SECTION,  "typeof new Object(null)",      "object",           typeof new Object(null) );
new TestCase( SECTION,  "MYOB = new Object(null); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Object]",   eval("MYOB = new Object(null); MYOB.toString = Object.prototype.toString; MYOB.toString()") );

new TestCase( SECTION,  "typeof new Object(void 0)",      "object",           typeof new Object(void 0) );
new TestCase( SECTION,  "MYOB = new Object(new Object(void 0)); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Object]",   eval("MYOB = new Object(new Object(void 0)); MYOB.toString = Object.prototype.toString; MYOB.toString()") );

new TestCase( SECTION,  "typeof new Object('string')",      "object",           typeof new Object('string') );
new TestCase( SECTION,  "MYOB = (new Object('string'); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object String]",   eval("MYOB = new Object('string'); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
new TestCase( SECTION,  "(new Object('string').valueOf()",  "string",           (new Object('string')).valueOf() );

new TestCase( SECTION,  "typeof new Object('')",            "object",           typeof new Object('') );
new TestCase( SECTION,  "MYOB = (new Object(''); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object String]",   eval("MYOB = new Object(''); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
new TestCase( SECTION,  "(new Object('').valueOf()",        "",                 (new Object('')).valueOf() );

new TestCase( SECTION,  "typeof new Object(Number.NaN)",      "object",                 typeof new Object(Number.NaN) );
new TestCase( SECTION,  "MYOB = (new Object(Number.NaN); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Number]",   eval("MYOB = new Object(Number.NaN); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
new TestCase( SECTION,  "(new Object(Number.NaN).valueOf()",  Number.NaN,               (new Object(Number.NaN)).valueOf() );

new TestCase( SECTION,  "typeof new Object(0)",      "object",                 typeof new Object(0) );
new TestCase( SECTION,  "MYOB = (new Object(0); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Number]",   eval("MYOB = new Object(0); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
new TestCase( SECTION,  "(new Object(0).valueOf()",  0,               (new Object(0)).valueOf() );

new TestCase( SECTION,  "typeof new Object(-0)",      "object",                 typeof new Object(-0) );
new TestCase( SECTION,  "MYOB = (new Object(-0); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Number]",   eval("MYOB = new Object(-0); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
new TestCase( SECTION,  "(new Object(-0).valueOf()",  -0,               (new Object(-0)).valueOf() );

new TestCase( SECTION,  "typeof new Object(1)",      "object",                 typeof new Object(1) );
new TestCase( SECTION,  "MYOB = (new Object(1); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Number]",   eval("MYOB = new Object(1); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
new TestCase( SECTION,  "(new Object(1).valueOf()",  1,               (new Object(1)).valueOf() );

new TestCase( SECTION,  "typeof new Object(-1)",      "object",                 typeof new Object(-1) );
new TestCase( SECTION,  "MYOB = (new Object(-1); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Number]",   eval("MYOB = new Object(-1); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
new TestCase( SECTION,  "(new Object(-1).valueOf()",  -1,               (new Object(-1)).valueOf() );

new TestCase( SECTION,  "typeof new Object(true)",      "object",                 typeof new Object(true) );
new TestCase( SECTION,  "MYOB = (new Object(true); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Boolean]",   eval("MYOB = new Object(true); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
new TestCase( SECTION,  "(new Object(true).valueOf()",  true,               (new Object(true)).valueOf() );

new TestCase( SECTION,  "typeof new Object(false)",      "object",              typeof new Object(false) );
new TestCase( SECTION,  "MYOB = (new Object(false); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Boolean]",   eval("MYOB = new Object(false); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
new TestCase( SECTION,  "(new Object(false).valueOf()",  false,                 (new Object(false)).valueOf() );

new TestCase( SECTION,  "typeof new Object(Boolean())",         "object",               typeof new Object(Boolean()) );
new TestCase( SECTION,  "MYOB = (new Object(Boolean()); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Boolean]",   eval("MYOB = new Object(Boolean()); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
new TestCase( SECTION,  "(new Object(Boolean()).valueOf()",     Boolean(),              (new Object(Boolean())).valueOf() );


var myglobal    = this;
var myobject    = new Object( "my new object" );
var myarray     = new Array();
var myboolean   = new Boolean();
var mynumber    = new Number();
var mystring    = new String();
var myobject    = new Object();
var myfunction  = new Function( "x", "return x");
var mymath      = Math;

new TestCase( SECTION, "myglobal = new Object( this )",                     myglobal,       new Object(this) );
new TestCase( SECTION, "myobject = new Object('my new object'); new Object(myobject)",            myobject,       new Object(myobject) );
new TestCase( SECTION, "myarray = new Array(); new Object(myarray)",        myarray,        new Object(myarray) );
new TestCase( SECTION, "myboolean = new Boolean(); new Object(myboolean)",  myboolean,      new Object(myboolean) );
new TestCase( SECTION, "mynumber = new Number(); new Object(mynumber)",     mynumber,       new Object(mynumber) );
new TestCase( SECTION, "mystring = new String9); new Object(mystring)",     mystring,       new Object(mystring) );
new TestCase( SECTION, "myobject = new Object(); new Object(mynobject)",    myobject,       new Object(myobject) );
new TestCase( SECTION, "myfunction = new Function(); new Object(myfunction)", myfunction,   new Object(myfunction) );
new TestCase( SECTION, "mymath = Math; new Object(mymath)",                 mymath,         new Object(mymath) );

test();
