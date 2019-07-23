





































gTestfile = '9.8.1.js';





























































var SECTION = "9.8.1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " ToString applied to the Number type");

new TestCase( SECTION,    "Number.NaN",       "NaN",                  Number.NaN + "" );
new TestCase( SECTION,    "0",                "0",                    0 + "" );
new TestCase( SECTION,    "-0",               "0",                   -0 + "" );
new TestCase( SECTION,    "Number.POSITIVE_INFINITY", "Infinity",     Number.POSITIVE_INFINITY + "" );
new TestCase( SECTION,    "Number.NEGATIVE_INFINITY", "-Infinity",    Number.NEGATIVE_INFINITY + "" );
new TestCase( SECTION,    "-1",               "-1",                   -1 + "" );



new TestCase( SECTION,    "1",                    "1",                    1 + "" );
new TestCase( SECTION,    "10",                   "10",                   10 + "" );
new TestCase( SECTION,    "100",                  "100",                  100 + "" );
new TestCase( SECTION,    "1000",                 "1000",                 1000 + "" );
new TestCase( SECTION,    "10000",                "10000",                10000 + "" );
new TestCase( SECTION,    "10000000000",          "10000000000",          10000000000 + "" );
new TestCase( SECTION,    "10000000000000000000", "10000000000000000000", 10000000000000000000 + "" );
new TestCase( SECTION,    "100000000000000000000","100000000000000000000",100000000000000000000 + "" );

new TestCase( SECTION,    "12345",                    "12345",                    12345 + "" );
new TestCase( SECTION,    "1234567890",               "1234567890",               1234567890 + "" );

new TestCase( SECTION,    "-1",                       "-1",                       -1 + "" );
new TestCase( SECTION,    "-10",                      "-10",                      -10 + "" );
new TestCase( SECTION,    "-100",                     "-100",                     -100 + "" );
new TestCase( SECTION,    "-1000",                    "-1000",                    -1000 + "" );
new TestCase( SECTION,    "-1000000000",              "-1000000000",              -1000000000 + "" );
new TestCase( SECTION,    "-1000000000000000",        "-1000000000000000",        -1000000000000000 + "" );
new TestCase( SECTION,    "-100000000000000000000",   "-100000000000000000000",   -100000000000000000000 + "" );
new TestCase( SECTION,    "-1000000000000000000000",  "-1e+21",                   -1000000000000000000000 + "" );

new TestCase( SECTION,    "-12345",                    "-12345",                  -12345 + "" );
new TestCase( SECTION,    "-1234567890",               "-1234567890",             -1234567890 + "" );


new TestCase( SECTION,    "1.0000001",                "1.0000001",                1.0000001 + "" );





new TestCase( SECTION,    "1000000000000000000000",   "1e+21",             1000000000000000000000 + "" );
new TestCase( SECTION,    "10000000000000000000000",   "1e+22",            10000000000000000000000 + "" );



new TestCase( SECTION,    "1.2345",                    "1.2345",                  String( 1.2345));
new TestCase( SECTION,    "1.234567890",               "1.23456789",             String( 1.234567890 ));


new TestCase( SECTION,    ".12345",                   "0.12345",                String(.12345 )     );
new TestCase( SECTION,    ".012345",                  "0.012345",               String(.012345)     );
new TestCase( SECTION,    ".0012345",                 "0.0012345",              String(.0012345)    );
new TestCase( SECTION,    ".00012345",                "0.00012345",             String(.00012345)   );
new TestCase( SECTION,    ".000012345",               "0.000012345",            String(.000012345)  );
new TestCase( SECTION,    ".0000012345",              "0.0000012345",           String(.0000012345) );
new TestCase( SECTION,    ".00000012345",             "1.2345e-7",              String(.00000012345));

new TestCase( SECTION,    "-1e21",                    "-1e+21",                 String(-1e21) );

test();

