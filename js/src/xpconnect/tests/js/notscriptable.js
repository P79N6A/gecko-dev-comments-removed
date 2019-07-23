












































StartTest( "Get and Set Properties on a native object that is not scriptable" );
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
	undefined,
	testObject.newProperty );

	
	var result = delete testObject.newProperty;
	
	AddTestCase(
		"delete testObject.newProperty",
		false,
		result );
	
	AddTestCase(
		"delete testObject.newProperty; testObject.newProperty",
		undefined,
		testObject.newProperty );

	

	testObject.newFunction = new Function( "return \"PASSED\"" );
	
	AddTestCase(
		"testObject.newFunction = new Function(\"return 'PASSED'\"); " +
		"typeof testObject.newFunction",
		"undefined",
		typeof testObject.newFunction );

	

	result = delete testObject.newFunction;
	AddTestCase(
		"delete testObject.newFunction",
		false,
		result);

	AddTestCase(
		"typeof testObject.newFunction",
		"undefined",
		typeof testObject.newFunction);

}