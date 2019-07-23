





































gTestfile = '7.7.3-1.js';


















var SECTION = "7.7.3-1";
var VERSION = "ECMA_1";
var TITLE   = "Numeric Literals";
var BUGNUMBER="122877";

startTest();

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "0x12345678",
	      305419896,
	      0x12345678 );

new TestCase( SECTION,
	      "0x80000000",
	      2147483648,
	      0x80000000 );

new TestCase( SECTION,
	      "0xffffffff",
	      4294967295,
	      0xffffffff );

new TestCase( SECTION,
	      "0x100000000",
	      4294967296,
	      0x100000000 );

new TestCase( SECTION,
	      "077777777777777777",
	      2251799813685247,
	      077777777777777777 );

new TestCase( SECTION,
	      "077777777777777776",
	      2251799813685246,
	      077777777777777776 );

new TestCase( SECTION,
	      "0x1fffffffffffff",
	      9007199254740991,
	      0x1fffffffffffff );

new TestCase( SECTION,
	      "0x20000000000000",
	      9007199254740992,
	      0x20000000000000 );

new TestCase( SECTION,
	      "0x20123456789abc",
	      9027215253084860,
	      0x20123456789abc );

new TestCase( SECTION,
	      "0x20123456789abd",
	      9027215253084860,
	      0x20123456789abd );

new TestCase( SECTION,
	      "0x20123456789abe",
	      9027215253084862,
	      0x20123456789abe );

new TestCase( SECTION,
	      "0x20123456789abf",
	      9027215253084864,
	      0x20123456789abf );

new TestCase( SECTION,
	      "0x1000000000000080",
	      1152921504606847000,
	      0x1000000000000080 );

new TestCase( SECTION,
	      "0x1000000000000081",
	      1152921504606847200,
	      0x1000000000000081 );

new TestCase( SECTION,
	      "0x1000000000000100",
	      1152921504606847200,
	      0x1000000000000100 );

new TestCase( SECTION,
	      "0x100000000000017f",
	      1152921504606847200,
	      0x100000000000017f );

new TestCase( SECTION,
	      "0x1000000000000180",
	      1152921504606847500,
	      0x1000000000000180 );

new TestCase( SECTION,
	      "0x1000000000000181",
	      1152921504606847500,
	      0x1000000000000181 );

new TestCase( SECTION,
	      "0x10000000000001f0",
	      1152921504606847500,
	      0x10000000000001f0 );

new TestCase( SECTION,
	      "0x1000000000000200",
	      1152921504606847500,
	      0x1000000000000200 );

new TestCase( SECTION,
	      "0x100000000000027f",
	      1152921504606847500,
	      0x100000000000027f );

new TestCase( SECTION,
	      "0x1000000000000280",
	      1152921504606847500,
	      0x1000000000000280 );

new TestCase( SECTION,
	      "0x1000000000000281",
	      1152921504606847700,
	      0x1000000000000281 );

new TestCase( SECTION,
	      "0x10000000000002ff",
	      1152921504606847700,
	      0x10000000000002ff );

new TestCase( SECTION,
	      "0x1000000000000300",
	      1152921504606847700,
	      0x1000000000000300 );

new TestCase( SECTION,
	      "0x10000000000000000",
	      18446744073709552000,
	      0x10000000000000000 );

test();

