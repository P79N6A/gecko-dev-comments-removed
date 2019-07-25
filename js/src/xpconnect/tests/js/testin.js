






































 




  StartTest( "Passing different types in" );

  var CLASS = Components.classes["@mozilla.org/js/xpc/test/In;1"];
  var IFACE = Components.interfaces["nsIXPCTestIn" ];

  var C = CLASS.createInstance();
  var c = C.QueryInterface(IFACE);

  
  

  TestLong( 
	[0, 0,   true, 1,    "A", 0,    new Boolean, 0,   {}, 0,
	 [],0,   new Date(987654321),987654321,  new Number(2134), 2134,
	 -987654321, -987654321,  undefined, 0,   null, 0,  void 0, 0,
	 NaN, 0, Function, 0  ]
  );

  TestLong(
	[0.2134, 0,   -0.2134, 0,     Math.pow(2,16)-1, Math.pow(2,16)-1,
	Math.pow(2,31)-1, Math.pow(2,31)-1,    Math.pow(2,31), -Math.pow(2,31),
	-(Math.pow(2,31)),   (-Math.pow(2,31)),  
	(-Math.pow(2,31))+1, (-Math.pow(2,31))+1,
	(-Math.pow(2,31))-1, (Math.pow(2,31))-1
	] );

  TestShort( [
    new Number(Math.pow(2,15)-1), Math.pow(2,15)-1,
    Math.pow(2,15)-1, Math.pow(2,15)-1,
	Math.pow(2,15), -Math.pow(2,15)
  ]);

  TestChar( [
	"A", "A",
	1,	 "1",
	255, "2",
	"XPConnect", "X",
	String.fromCharCode(Math.pow(2,16)), String.fromCharCode(Math.pow(2,16))
  ]);

  TestBoolean( [
  ]);
  TestOctet( [
  ]);
  TestLongLong( [
  ]);
  TestUnsignedShort( [
  ]);
  TestUnsignedLong( [
  ]);
  TestFloat( [
  ]);
  TestDouble( [
  ]);
  TestWchar([
  ]);
  TestString([
  ]);
  TestPRBool( [
  ]);
  TestPRInt32( [
  ]);
  TestPRInt16( [
  ]);
  TestPrInt64( [
  ]);
  TestPRUint8( [
    new Number(Math.pow(2,8)), 0,
    Math.pow(2,8)-1, Math.pow(2,8)-1,
	Math.pow(2,8), 0,
	-Math.pow(2,8), 0,

  ]);
  TestPRUint16( [
  ]);
  TestPRUint32( [
  ]);
  TestPRUint64( [
  ]);

  TestVoidStar( [
  ]);

  TestCharStar( [
  ]);

  StopTest();

  function TestLong( data ) {
    for ( var i = 0; i < data.length; i+=2 ) {
		AddTestCase( "c.EchoLong("+data[i]+")",
			data[i+1],
			c.EchoLong(data[i]));
	}
  }
  function TestShort( data ) {
    for ( var i = 0; i < data.length; i+=2 ) {
		AddTestCase( "c.EchoShort("+data[i]+")",
			data[i+1],
			c.EchoShort(data[i]));
	}
  }
  function TestChar( data ) {
    for ( var i = 0; i < data.length; i+=2 ) {
		AddTestCase( "c.EchoChar("+data[i]+")",
			data[i+1],
			c.EchoChar(data[i]));
	}
  }
  function TestBoolean() {
  }
  
  function TestOctet() {
  }
  function TestLongLong() {
  }
  function TestUnsignedShort() {
  }
  function TestUnsignedLong() {
  }
  function TestFloat() {
  }
  function TestDouble() {
  }
  function TestWchar() {
  }
  function TestString() {
  }
  function TestPRBool() {
  }
  function TestPRInt32() {
  }
  function TestPRInt16() {
  }
  function TestPrInt64() {
  }
  function TestPRUint16() {
  }
  function TestPRUint32() {
  }
  function TestPRUint64() {
  }
  function TestPRUint8( data ) {
    for ( var i = 0; i < data.length; i+=2 ) {
		AddTestCase( "c.EchoPRUint8("+data[i]+")",
			data[i+1],
			c.EchoPRUint8(data[i]));
	}
  }
  function TestVoidStar( data ) {
	for ( var i = 0; i < data.length; i+=2 ) {
		AddTestCase( "c.EchoVoidStar("+data[i]+")",
			data[i+1],
			c.EchoVoidStar(data[i]));
	}
  }
  function TestCharStar() {
  }

