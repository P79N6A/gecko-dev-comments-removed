





































gTestfile = '15.2.1.1.js';




























var SECTION = "15.2.1.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Object( value )";

writeHeaderToLog( SECTION + " "+ TITLE);


var NULL_OBJECT = Object(null);

new TestCase( SECTION, "Object(null).valueOf()",    NULL_OBJECT,           (NULL_OBJECT).valueOf() );
new TestCase( SECTION, "typeof Object(null)",       "object",               typeof (Object(null)) );

var UNDEFINED_OBJECT = Object( void 0 );

new TestCase( SECTION, "Object(void 0).valueOf()",    UNDEFINED_OBJECT,           (UNDEFINED_OBJECT).valueOf() );
new TestCase( SECTION, "typeof Object(void 0)",       "object",               typeof (Object(void 0)) );

new TestCase( SECTION, "Object(true).valueOf()",    true,                   (Object(true)).valueOf() );
new TestCase( SECTION, "typeof Object(true)",       "object",               typeof Object(true) );
new TestCase( SECTION, "var MYOB = Object(true); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Boolean]",      eval("var MYOB = Object(true); MYOB.toString = Object.prototype.toString; MYOB.toString()") );

new TestCase( SECTION, "Object(false).valueOf()",    false,                  (Object(false)).valueOf() );
new TestCase( SECTION, "typeof Object(false)",      "object",               typeof Object(false) );
new TestCase( SECTION, "var MYOB = Object(false); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Boolean]",      eval("var MYOB = Object(false); MYOB.toString = Object.prototype.toString; MYOB.toString()") );

new TestCase( SECTION, "Object(0).valueOf()",       0,                      (Object(0)).valueOf() );
new TestCase( SECTION, "typeof Object(0)",          "object",               typeof Object(0) );
new TestCase( SECTION, "var MYOB = Object(0); MYOB.toString = Object.prototype.toString; MYOB.toString()",      "[object Number]",      eval("var MYOB = Object(0); MYOB.toString = Object.prototype.toString; MYOB.toString()") );

new TestCase( SECTION, "Object(-0).valueOf()",      -0,                     (Object(-0)).valueOf() );
new TestCase( SECTION, "typeof Object(-0)",         "object",               typeof Object(-0) );
new TestCase( SECTION, "var MYOB = Object(-0); MYOB.toString = Object.prototype.toString; MYOB.toString()",     "[object Number]",      eval("var MYOB = Object(-0); MYOB.toString = Object.prototype.toString; MYOB.toString()") );

new TestCase( SECTION, "Object(1).valueOf()",       1,                      (Object(1)).valueOf() );
new TestCase( SECTION, "typeof Object(1)",          "object",               typeof Object(1) );
new TestCase( SECTION, "var MYOB = Object(1); MYOB.toString = Object.prototype.toString; MYOB.toString()",     "[object Number]",      eval("var MYOB = Object(1); MYOB.toString = Object.prototype.toString; MYOB.toString()") );

new TestCase( SECTION, "Object(-1).valueOf()",      -1,                     (Object(-1)).valueOf() );
new TestCase( SECTION, "typeof Object(-1)",         "object",               typeof Object(-1) );
new TestCase( SECTION, "var MYOB = Object(-1); MYOB.toString = Object.prototype.toString; MYOB.toString()",     "[object Number]",      eval("var MYOB = Object(-1); MYOB.toString = Object.prototype.toString; MYOB.toString()") );

new TestCase( SECTION, "Object(Number.MAX_VALUE).valueOf()",    1.7976931348623157e308,         (Object(Number.MAX_VALUE)).valueOf() );
new TestCase( SECTION, "typeof Object(Number.MAX_VALUE)",       "object",                       typeof Object(Number.MAX_VALUE) );
new TestCase( SECTION, "var MYOB = Object(Number.MAX_VALUE); MYOB.toString = Object.prototype.toString; MYOB.toString()",     "[object Number]",      eval("var MYOB = Object(Number.MAX_VALUE); MYOB.toString = Object.prototype.toString; MYOB.toString()") );

new TestCase( SECTION, "Object(Number.MIN_VALUE).valueOf()",     5e-324,           (Object(Number.MIN_VALUE)).valueOf() );
new TestCase( SECTION, "typeof Object(Number.MIN_VALUE)",       "object",         typeof Object(Number.MIN_VALUE) );
new TestCase( SECTION, "var MYOB = Object(Number.MIN_VALUE); MYOB.toString = Object.prototype.toString; MYOB.toString()",     "[object Number]",      eval("var MYOB = Object(Number.MIN_VALUE); MYOB.toString = Object.prototype.toString; MYOB.toString()") );

new TestCase( SECTION, "Object(Number.POSITIVE_INFINITY).valueOf()",    Number.POSITIVE_INFINITY,       (Object(Number.POSITIVE_INFINITY)).valueOf() );
new TestCase( SECTION, "typeof Object(Number.POSITIVE_INFINITY)",       "object",                       typeof Object(Number.POSITIVE_INFINITY) );
new TestCase( SECTION, "var MYOB = Object(Number.POSITIVE_INFINITY); MYOB.toString = Object.prototype.toString; MYOB.toString()",     "[object Number]",      eval("var MYOB = Object(Number.POSITIVE_INFINITY); MYOB.toString = Object.prototype.toString; MYOB.toString()") );

new TestCase( SECTION, "Object(Number.NEGATIVE_INFINITY).valueOf()",    Number.NEGATIVE_INFINITY,       (Object(Number.NEGATIVE_INFINITY)).valueOf() );
new TestCase( SECTION, "typeof Object(Number.NEGATIVE_INFINITY)",       "object",            typeof Object(Number.NEGATIVE_INFINITY) );
new TestCase( SECTION, "var MYOB = Object(Number.NEGATIVE_INFINITY); MYOB.toString = Object.prototype.toString; MYOB.toString()",     "[object Number]",      eval("var MYOB = Object(Number.NEGATIVE_INFINITY); MYOB.toString = Object.prototype.toString; MYOB.toString()") );

new TestCase( SECTION, "Object(Number.NaN).valueOf()",      Number.NaN,                (Object(Number.NaN)).valueOf() );
new TestCase( SECTION, "typeof Object(Number.NaN)",         "object",                  typeof Object(Number.NaN) );
new TestCase( SECTION, "var MYOB = Object(Number.NaN); MYOB.toString = Object.prototype.toString; MYOB.toString()",     "[object Number]",      eval("var MYOB = Object(Number.NaN); MYOB.toString = Object.prototype.toString; MYOB.toString()") );

new TestCase( SECTION, "Object('a string').valueOf()",      "a string",         (Object("a string")).valueOf() );
new TestCase( SECTION, "typeof Object('a string')",         "object",           typeof (Object("a string")) );
new TestCase( SECTION, "var MYOB = Object('a string'); MYOB.toString = Object.prototype.toString; MYOB.toString()",     "[object String]",      eval("var MYOB = Object('a string'); MYOB.toString = Object.prototype.toString; MYOB.toString()") );

new TestCase( SECTION, "Object('').valueOf()",              "",                 (Object("")).valueOf() );
new TestCase( SECTION, "typeof Object('')",                 "object",           typeof (Object("")) );
new TestCase( SECTION, "var MYOB = Object(''); MYOB.toString = Object.prototype.toString; MYOB.toString()",     "[object String]",      eval("var MYOB = Object(''); MYOB.toString = Object.prototype.toString; MYOB.toString()") );

new TestCase( SECTION, "Object('\\r\\t\\b\\n\\v\\f').valueOf()",   "\r\t\b\n\v\f",   (Object("\r\t\b\n\v\f")).valueOf() );
new TestCase( SECTION, "typeof Object('\\r\\t\\b\\n\\v\\f')",      "object",           typeof (Object("\\r\\t\\b\\n\\v\\f")) );
new TestCase( SECTION, "var MYOB = Object('\\r\\t\\b\\n\\v\\f'); MYOB.toString = Object.prototype.toString; MYOB.toString()",     "[object String]",      eval("var MYOB = Object('\\r\\t\\b\\n\\v\\f'); MYOB.toString = Object.prototype.toString; MYOB.toString()") );

new TestCase( SECTION,  "Object( '\\\'\\\"\\' ).valueOf()",      "\'\"\\",          (Object("\'\"\\")).valueOf() );
new TestCase( SECTION,  "typeof Object( '\\\'\\\"\\' )",        "object",           typeof Object("\'\"\\") );


test();
