














































StartTest( "More inheritance tests" );
SetupTest();
AddTestData();
StopTest();

function SetupTest() {
try {		
	
	
	
	

	iChild = Components.interfaces.nsIXPCTestChild5;
	iParentOne = Components.interfaces.nsIXPCTestParentOne;
	iParentTwo = Components.interfaces.nsIXPCTestParentTwo;

	CONTRACTID = "@mozilla.org/js/xpc/test/Child5;1";
	cChild = Components.classes[CONTRACTID].createInstance();
	CONTRACTID = "@mozilla.org/js/xpc/test/ParentOne;1";
	cParentOne = Components.classes[CONTRACTID].createInstance();
	CONTRACTID = "@mozilla.org/js/xpc/test/ParentTwo;1";
	cParentTwo = Components.classes[CONTRACTID].createInstance();

	c_c5 = cChild.QueryInterface(iChild);
	c_p2 = cChild.QueryInterface(iParentTwo);

	parentOne = cParentOne.QueryInterface(iParentOne);
	parentTwo = cParentTwo.QueryInterface(iParentTwo);

} catch (e) {
	WriteLine(e);
	AddTestCase(
		"Setting up the test",
		"PASSED",
		"FAILED: " + e.message  +" "+ e.location +". ");
}
}

function AddTestData() {
	Check( parentOne, "@mozilla.org/js/xpc/test/Child5;1", c_c5 );
	Check( cChild,    "@mozilla.org/js/xpc/test/Child5;1", c_c5 );
	Check( parentTwo, "@mozilla.org/js/xpc/test/ParentTwo;1", c_p2 );

}

function Check(parent,parentName,child) {
	

	parentProps = {};
	for ( p in parent ) parentProps[p] = true;
	for ( p in child ) if ( parent[p] ) parentProps[p] = false;

	for ( p in parentProps ) {
		AddTestCase(
			"child has property " + p,
			true,
			(parentProps[p] ? false : true ) 
		);

		if ( p.match(/Method/) ) {
			AddTestCase(
				"child."+p+"()",
				parentName +" method",
				child[p]() );

		} else if (p.match(/Attribute/)) {
			AddTestCase(
				"child." +p,
				parentName +" attribute",
				child[p] );
		}
	}

	var result = true;
	for ( p in parentProps ) {
		if ( parentProps[p] == true )
			result == false;
	}

	AddTestCase(
		"child has all parent properties?",
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
				parentName +" method",
				child[p]() );

		} else if (p.match(/Attribute/)) {
			AddTestCase(
				"child." +p,
				parentName +" attribute",
				child[p] );
		}
	}		
}
