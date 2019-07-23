





































gTestfile = '11.2.1-1.js';











































var SECTION = "11.2.1-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Property Accessors";
writeHeaderToLog( SECTION + " "+TITLE );



var PROPERTY = new Array();
var p = 0;



PROPERTY[p++] = new Property( "this",   "NaN",          "number" );
PROPERTY[p++] = new Property( "this",   "Infinity",     "number" );
PROPERTY[p++] = new Property( "this",   "eval",         "function" );
PROPERTY[p++] = new Property( "this",   "parseInt",     "function" );
PROPERTY[p++] = new Property( "this",   "parseFloat",   "function" );
PROPERTY[p++] = new Property( "this",   "escape",       "function" );
PROPERTY[p++] = new Property( "this",   "unescape",     "function" );
PROPERTY[p++] = new Property( "this",   "isNaN",        "function" );
PROPERTY[p++] = new Property( "this",   "isFinite",     "function" );
PROPERTY[p++] = new Property( "this",   "Object",       "function" );
PROPERTY[p++] = new Property( "this",   "Number",       "function" );
PROPERTY[p++] = new Property( "this",   "Function",     "function" );
PROPERTY[p++] = new Property( "this",   "Array",        "function" );
PROPERTY[p++] = new Property( "this",   "String",       "function" );
PROPERTY[p++] = new Property( "this",   "Boolean",      "function" );
PROPERTY[p++] = new Property( "this",   "Date",         "function" );
PROPERTY[p++] = new Property( "this",   "Math",         "object" );



PROPERTY[p++] = new Property( "Object", "prototype",    "object" );
PROPERTY[p++] = new Property( "Object", "toString",     "function" );
PROPERTY[p++] = new Property( "Object", "valueOf",      "function" );
PROPERTY[p++] = new Property( "Object", "constructor",  "function" );



PROPERTY[p++] = new Property( "Function",   "prototype",    "function" );
PROPERTY[p++] = new Property( "Function.prototype",   "toString",     "function" );
PROPERTY[p++] = new Property( "Function.prototype",   "length",       "number" );
PROPERTY[p++] = new Property( "Function.prototype",   "valueOf",      "function" );

Function.prototype.myProperty = "hi";

PROPERTY[p++] = new Property( "Function.prototype",   "myProperty",   "string" );


PROPERTY[p++] = new Property( "Array",      "prototype",    "object" );
PROPERTY[p++] = new Property( "Array",      "length",       "number" );
PROPERTY[p++] = new Property( "Array.prototype",      "constructor",  "function" );
PROPERTY[p++] = new Property( "Array.prototype",      "toString",     "function" );
PROPERTY[p++] = new Property( "Array.prototype",      "join",         "function" );
PROPERTY[p++] = new Property( "Array.prototype",      "reverse",      "function" );
PROPERTY[p++] = new Property( "Array.prototype",      "sort",         "function" );


PROPERTY[p++] = new Property( "String",     "prototype",    "object" );
PROPERTY[p++] = new Property( "String",     "fromCharCode", "function" );
PROPERTY[p++] = new Property( "String.prototype",     "toString",     "function" );
PROPERTY[p++] = new Property( "String.prototype",     "constructor",  "function" );
PROPERTY[p++] = new Property( "String.prototype",     "valueOf",      "function" );
PROPERTY[p++] = new Property( "String.prototype",     "charAt",       "function" );
PROPERTY[p++] = new Property( "String.prototype",     "charCodeAt",   "function" );
PROPERTY[p++] = new Property( "String.prototype",     "indexOf",      "function" );
PROPERTY[p++] = new Property( "String.prototype",     "lastIndexOf",  "function" );
PROPERTY[p++] = new Property( "String.prototype",     "split",        "function" );
PROPERTY[p++] = new Property( "String.prototype",     "substring",    "function" );
PROPERTY[p++] = new Property( "String.prototype",     "toLowerCase",  "function" );
PROPERTY[p++] = new Property( "String.prototype",     "toUpperCase",  "function" );
PROPERTY[p++] = new Property( "String.prototype",     "length",       "number" );


PROPERTY[p++] = new Property( "Boolean",    "prototype",    "object" );
PROPERTY[p++] = new Property( "Boolean",    "constructor",  "function" );
PROPERTY[p++] = new Property( "Boolean.prototype",    "valueOf",      "function" );
PROPERTY[p++] = new Property( "Boolean.prototype",    "toString",     "function" );



PROPERTY[p++] = new Property( "Number",     "MAX_VALUE",    "number" );
PROPERTY[p++] = new Property( "Number",     "MIN_VALUE",    "number" );
PROPERTY[p++] = new Property( "Number",     "NaN",          "number" );
PROPERTY[p++] = new Property( "Number",     "NEGATIVE_INFINITY",    "number" );
PROPERTY[p++] = new Property( "Number",     "POSITIVE_INFINITY",    "number" );
PROPERTY[p++] = new Property( "Number.prototype",     "toString",     "function" );
PROPERTY[p++] = new Property( "Number.prototype",     "constructor",  "function" );
PROPERTY[p++] = new Property( "Number.prototype",     "valueOf",        "function" );


PROPERTY[p++] = new Property( "Math",   "E",        "number" );
PROPERTY[p++] = new Property( "Math",   "LN10",     "number" );
PROPERTY[p++] = new Property( "Math",   "LN2",      "number" );
PROPERTY[p++] = new Property( "Math",   "LOG2E",    "number" );
PROPERTY[p++] = new Property( "Math",   "LOG10E",   "number" );
PROPERTY[p++] = new Property( "Math",   "PI",       "number" );
PROPERTY[p++] = new Property( "Math",   "SQRT1_2",  "number" );
PROPERTY[p++] = new Property( "Math",   "SQRT2",    "number" );
PROPERTY[p++] = new Property( "Math",   "abs",      "function" );
PROPERTY[p++] = new Property( "Math",   "acos",     "function" );
PROPERTY[p++] = new Property( "Math",   "asin",     "function" );
PROPERTY[p++] = new Property( "Math",   "atan",     "function" );
PROPERTY[p++] = new Property( "Math",   "atan2",    "function" );
PROPERTY[p++] = new Property( "Math",   "ceil",     "function" );
PROPERTY[p++] = new Property( "Math",   "cos",      "function" );
PROPERTY[p++] = new Property( "Math",   "exp",      "function" );
PROPERTY[p++] = new Property( "Math",   "floor",    "function" );
PROPERTY[p++] = new Property( "Math",   "log",      "function" );
PROPERTY[p++] = new Property( "Math",   "max",      "function" );
PROPERTY[p++] = new Property( "Math",   "min",      "function" );
PROPERTY[p++] = new Property( "Math",   "pow",      "function" );
PROPERTY[p++] = new Property( "Math",   "random",   "function" );
PROPERTY[p++] = new Property( "Math",   "round",    "function" );
PROPERTY[p++] = new Property( "Math",   "sin",      "function" );
PROPERTY[p++] = new Property( "Math",   "sqrt",     "function" );
PROPERTY[p++] = new Property( "Math",   "tan",      "function" );


PROPERTY[p++] = new Property( "Date",   "parse",        "function" );
PROPERTY[p++] = new Property( "Date",   "prototype",    "object" );
PROPERTY[p++] = new Property( "Date",   "UTC",          "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "constructor",    "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "toString",       "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "valueOf",        "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "getTime",        "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "getYear",        "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "getFullYear",    "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "getUTCFullYear", "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "getMonth",       "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "getUTCMonth",    "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "getDate",        "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "getUTCDate",     "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "getDay",         "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "getUTCDay",      "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "getHours",       "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "getUTCHours",    "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "getMinutes",     "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "getUTCMinutes",  "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "getSeconds",     "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "getUTCSeconds",  "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "getMilliseconds","function" );
PROPERTY[p++] = new Property( "Date.prototype",   "getUTCMilliseconds", "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "setTime",        "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "setMilliseconds","function" );
PROPERTY[p++] = new Property( "Date.prototype",   "setUTCMilliseconds", "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "setSeconds",     "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "setUTCSeconds",  "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "setMinutes",     "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "setUTCMinutes",  "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "setHours",       "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "setUTCHours",    "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "setDate",        "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "setUTCDate",     "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "setMonth",       "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "setUTCMonth",    "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "setFullYear",    "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "setUTCFullYear", "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "setYear",        "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "toLocaleString", "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "toUTCString",    "function" );
PROPERTY[p++] = new Property( "Date.prototype",   "toGMTString",    "function" );

for ( var i = 0, RESULT; i < PROPERTY.length; i++ ) {
  RESULT = eval("typeof " + PROPERTY[i].object + "." + PROPERTY[i].name );

  new TestCase( SECTION,
                "typeof " + PROPERTY[i].object + "." + PROPERTY[i].name,
                PROPERTY[i].type,
                RESULT );

  RESULT = eval("typeof " + PROPERTY[i].object + "['" + PROPERTY[i].name +"']");

  new TestCase( SECTION,
                "typeof " + PROPERTY[i].object + "['" + PROPERTY[i].name +"']",
                PROPERTY[i].type,
                RESULT );
}

test();

function MyObject( arg0, arg1, arg2, arg3, arg4 ) {
  this.name   = arg0;
}
function Property( object, name, type ) {
  this.object = object;
  this.name = name;
  this.type = type;
}
