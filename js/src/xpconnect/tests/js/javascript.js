












































StartTest( "Get and Set Properties on a native object" );
SetupTest();
AddTestData();
StopTest();

function SetupTest() {
	CONTRACTID = "@mozilla.org/js/xpc/test/ObjectReadWrite;1";
	CLASS = Components.classes[CONTRACTID].createInstance();
	IFACE = Components.interfaces.nsIXPCTestObjectReadWrite;

	testObject = CLASS.QueryInterface(IFACE);
}

function AddTestData() {
	

	testObject.newProperty = "PASS",
	AddTestCase(
	"testObject.newProperty = \"PASS\"; testObject.newProperty",
	"PASS",
	testObject.newProperty );

	
	var result = delete testObject.newProperty;
	
	AddTestCase(
		"delete testObject.newProperty",
		true,
		result );
	AddTestCase(
		"delete testObject.newProperty; testObject.newProperty",
		undefined,
		testObject.newProperty );

	

	testObject.newFunction = new Function( "return \"PASSED\"" );
	
	AddTestCase(
		"testObject.newFunction = new Function(\"return 'PASSED'\"); " +
		"typeof testObject.newFunction",
		"function",
		typeof testObject.newFunction );

	var s = "testObject.newFunction()"

	AddTestCase(
		"testObject.newFunction()",
		"PASSED",
		eval(s));
}
