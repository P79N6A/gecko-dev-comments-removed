





































gTestfile = '15.2.1.1.js';




























var SECTION = "15.2.1.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Object( value )";

writeHeaderToLog( SECTION + " "+ TITLE);


var NULL_OBJECT = Object(null);

new TestCase( SECTION, "Object(null).__proto__",    Object.prototype,       (Object(null)).__proto__ );

new TestCase( SECTION, "Object(void 0).__proto__",    Object.prototype,       (Object(void 0)).__proto__ );

test();
