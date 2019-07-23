





































gTestfile = '11.2.3-1.js';




































var SECTION = "11.2.3-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Function Calls";

writeHeaderToLog( SECTION + " "+ TITLE);
















var OBJECT = true;

new TestCase( SECTION,
              "OBJECT.toString()",
              "true",
              OBJECT.toString() );



new TestCase( SECTION,
              "(new Array())['length'].valueOf()",
              0,
              (new Array())["length"].valueOf() );


new TestCase( SECTION,
              "(new Array()).length.valueOf()",
              0,
              (new Array()).length.valueOf() );


new TestCase( SECTION,
              "(new Array(20))['length'].valueOf()",
              20,
              (new Array(20))["length"].valueOf() );

test();

