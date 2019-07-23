





































gTestfile = '15.7.2.js';




























var SECTION = "15.7.2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The Number Constructor";

writeHeaderToLog( SECTION + " "+ TITLE);




new TestCase(SECTION, "(new Number()).constructor",      Number.prototype.constructor,   (new Number()).constructor );

new TestCase(SECTION, "typeof (new Number())",         "object",           typeof (new Number()) );
new TestCase(SECTION,  "(new Number()).valueOf()",     0,                   (new Number()).valueOf() );
new TestCase(SECTION,
	     "NUMB = new Number();NUMB.toString=Object.prototype.toString;NUMB.toString()",
	     "[object Number]",
	     eval("NUMB = new Number();NUMB.toString=Object.prototype.toString;NUMB.toString()") );

new TestCase(SECTION, "(new Number(0)).constructor",     Number.prototype.constructor,    (new Number(0)).constructor );
new TestCase(SECTION, "typeof (new Number(0))",         "object",           typeof (new Number(0)) );
new TestCase(SECTION,  "(new Number(0)).valueOf()",     0,                   (new Number(0)).valueOf() );
new TestCase(SECTION,
	     "NUMB = new Number(0);NUMB.toString=Object.prototype.toString;NUMB.toString()",
	     "[object Number]",
	     eval("NUMB = new Number(0);NUMB.toString=Object.prototype.toString;NUMB.toString()") );

new TestCase(SECTION, "(new Number(1)).constructor",     Number.prototype.constructor,    (new Number(1)).constructor );
new TestCase(SECTION, "typeof (new Number(1))",         "object",           typeof (new Number(1)) );
new TestCase(SECTION,  "(new Number(1)).valueOf()",     1,                   (new Number(1)).valueOf() );
new TestCase(SECTION,
	     "NUMB = new Number(1);NUMB.toString=Object.prototype.toString;NUMB.toString()",
	     "[object Number]",
	     eval("NUMB = new Number(1);NUMB.toString=Object.prototype.toString;NUMB.toString()") );

new TestCase(SECTION, "(new Number(-1)).constructor",     Number.prototype.constructor,    (new Number(-1)).constructor );
new TestCase(SECTION, "typeof (new Number(-1))",         "object",           typeof (new Number(-1)) );
new TestCase(SECTION,  "(new Number(-1)).valueOf()",     -1,                   (new Number(-1)).valueOf() );
new TestCase(SECTION,
	     "NUMB = new Number(-1);NUMB.toString=Object.prototype.toString;NUMB.toString()",
	     "[object Number]",
	     eval("NUMB = new Number(-1);NUMB.toString=Object.prototype.toString;NUMB.toString()") );

new TestCase(SECTION, "(new Number(Number.NaN)).constructor",     Number.prototype.constructor,    (new Number(Number.NaN)).constructor );
new TestCase(SECTION, "typeof (new Number(Number.NaN))",         "object",           typeof (new Number(Number.NaN)) );
new TestCase(SECTION,  "(new Number(Number.NaN)).valueOf()",     Number.NaN,                   (new Number(Number.NaN)).valueOf() );
new TestCase(SECTION,
	     "NUMB = new Number(Number.NaN);NUMB.toString=Object.prototype.toString;NUMB.toString()",
	     "[object Number]",
	     eval("NUMB = new Number(Number.NaN);NUMB.toString=Object.prototype.toString;NUMB.toString()") );

new TestCase(SECTION, "(new Number('string')).constructor",     Number.prototype.constructor,    (new Number('string')).constructor );
new TestCase(SECTION, "typeof (new Number('string'))",         "object",           typeof (new Number('string')) );
new TestCase(SECTION,  "(new Number('string')).valueOf()",     Number.NaN,                   (new Number('string')).valueOf() );
new TestCase(SECTION,
	     "NUMB = new Number('string');NUMB.toString=Object.prototype.toString;NUMB.toString()",
	     "[object Number]",
	     eval("NUMB = new Number('string');NUMB.toString=Object.prototype.toString;NUMB.toString()") );

new TestCase(SECTION, "(new Number(new String())).constructor",     Number.prototype.constructor,    (new Number(new String())).constructor );
new TestCase(SECTION, "typeof (new Number(new String()))",         "object",           typeof (new Number(new String())) );
new TestCase(SECTION,  "(new Number(new String())).valueOf()",     0,                   (new Number(new String())).valueOf() );
new TestCase(SECTION,
	     "NUMB = new Number(new String());NUMB.toString=Object.prototype.toString;NUMB.toString()",
	     "[object Number]",
	     eval("NUMB = new Number(new String());NUMB.toString=Object.prototype.toString;NUMB.toString()") );

new TestCase(SECTION, "(new Number('')).constructor",     Number.prototype.constructor,    (new Number('')).constructor );
new TestCase(SECTION, "typeof (new Number(''))",         "object",           typeof (new Number('')) );
new TestCase(SECTION,  "(new Number('')).valueOf()",     0,                   (new Number('')).valueOf() );
new TestCase(SECTION,
	     "NUMB = new Number('');NUMB.toString=Object.prototype.toString;NUMB.toString()",
	     "[object Number]",
	     eval("NUMB = new Number('');NUMB.toString=Object.prototype.toString;NUMB.toString()") );

new TestCase(SECTION, "(new Number(Number.POSITIVE_INFINITY)).constructor",     Number.prototype.constructor,    (new Number(Number.POSITIVE_INFINITY)).constructor );
new TestCase(SECTION, "typeof (new Number(Number.POSITIVE_INFINITY))",         "object",           typeof (new Number(Number.POSITIVE_INFINITY)) );
new TestCase(SECTION,  "(new Number(Number.POSITIVE_INFINITY)).valueOf()",     Number.POSITIVE_INFINITY,    (new Number(Number.POSITIVE_INFINITY)).valueOf() );
new TestCase(SECTION,
	     "NUMB = new Number(Number.POSITIVE_INFINITY);NUMB.toString=Object.prototype.toString;NUMB.toString()",
	     "[object Number]",
	     eval("NUMB = new Number(Number.POSITIVE_INFINITY);NUMB.toString=Object.prototype.toString;NUMB.toString()") );

new TestCase(SECTION, "(new Number(Number.NEGATIVE_INFINITY)).constructor",     Number.prototype.constructor,    (new Number(Number.NEGATIVE_INFINITY)).constructor );
new TestCase(SECTION, "typeof (new Number(Number.NEGATIVE_INFINITY))",         "object",           typeof (new Number(Number.NEGATIVE_INFINITY)) );
new TestCase(SECTION,  "(new Number(Number.NEGATIVE_INFINITY)).valueOf()",     Number.NEGATIVE_INFINITY,                   (new Number(Number.NEGATIVE_INFINITY)).valueOf() );
new TestCase(SECTION,
	     "NUMB = new Number(Number.NEGATIVE_INFINITY);NUMB.toString=Object.prototype.toString;NUMB.toString()",
	     "[object Number]",
	     eval("NUMB = new Number(Number.NEGATIVE_INFINITY);NUMB.toString=Object.prototype.toString;NUMB.toString()") );


new TestCase(SECTION, "(new Number()).constructor",     Number.prototype.constructor,    (new Number()).constructor );
new TestCase(SECTION, "typeof (new Number())",         "object",           typeof (new Number()) );
new TestCase(SECTION,  "(new Number()).valueOf()",     0,                   (new Number()).valueOf() );
new TestCase(SECTION,
	     "NUMB = new Number();NUMB.toString=Object.prototype.toString;NUMB.toString()",
	     "[object Number]",
	     eval("NUMB = new Number();NUMB.toString=Object.prototype.toString;NUMB.toString()") );

test();
