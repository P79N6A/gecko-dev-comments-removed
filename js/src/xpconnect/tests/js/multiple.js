














































StartTest( "Interfaces that inherit from interfaces other than nsISupports" );
SetupTest();
AddTestData();
StopTest();

function SetupTest() {
try {		
	iChild = Components.interfaces.nsIXPCTestChild3;
	iParentOne = Components.interfaces.nsIXPCTestParentOne;

	CONTRACTID = "@mozilla.org/js/xpc/test/Child3;1";
	cChild = Components.classes[CONTRACTID].createInstance();
	CONTRACTID = "@mozilla.org/js/xpc/test/ParentOne;1";
	cParentOne = Components.classes[CONTRACTID].createInstance();

	child = cChild.QueryInterface(iChild);
	parentOne = cParentOne.QueryInterface(iParentOne);

} catch (e) {
	WriteLine(e);
	AddTestCase(
		"Setting up the test",
		"PASSED",
		"FAILED: " + e.message  +" "+ e.location +". ");
}
}

function AddTestData() {
	

	parentOneProps = {};
	for ( p in parentOne ) parentOneProps[p] = true;
	for ( p in child ) if ( parentOneProps[p] ) parentOneProps[p] = false;

	for ( p in parentOneProps ) {
		print( p );

		AddTestCase(
			"child has property " + p,
			true,
			(parentOneProps[p] ? false : true ) 
		);

		if ( p.match(/Method/) ) {
			AddTestCase(
				"child."+p+"()",
				"@mozilla.org/js/xpc/test/Child3;1 method",
				child[p]() );

		} else if (p.match(/Attribute/)) {
			AddTestCase(
				"child." +p,
				"@mozilla.org/js/xpc/test/Child3;1 attribute",
				child[p] );
		}
	}

	var result = true;
	for ( p in parentOneProps ) {
		if ( parentOneProps[p] = true )
			result == false;
	}

	AddTestCase(
		"child has parentOne properties?",
		true,
		result );

	for ( p in child ) {
		AddTestCase(
			"child has property " + p,
			true,
			(child[p] ? true : false ) 
		);

		if ( p.match(/Method/) ) {
			AddTestCase(
				"child."+p+"()",
				"@mozilla.org/js/xpc/test/Child3;1 method",
				child[p]() );

		} else if (p.match(/Attribute/)) {
			AddTestCase(
				"child." +p,
				"@mozilla.org/js/xpc/test/Child3;1 attribute",
				child[p] );
		}
	}		
}

