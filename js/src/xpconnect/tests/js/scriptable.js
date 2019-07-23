












































StartTest( "Get and Set Properties on a native object" );
SetupTest();
AddTestData();
StopTest();

function SetupTest() {
	CONTRACTID = "@mozilla.org/js/xpc/test/Overloaded;1";
	CLASS = Components.classes[CONTRACTID].createInstance();
	IFACE = Components.interfaces.nsIXPCTestScriptable;

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

	
	testObject.newFunction = new Function( "this.result = \"PASSED\"" );

	AddTestCase( 
		"testObject.newFunction = new Function( 'this.result = \"PASSED\"'); "+
		"new testObject.newFunction().result",
		"PASSED",
		new testObject.newFunction().result );


	

	result = delete testObject.newFunction;
	AddTestCase(
		"delete testObject.newFunction",
		true,
		result);

	AddTestCase(
		"typeof testObject.newFunction",
		"undefined",
		typeof testObject.newFunction);

}
